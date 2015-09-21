#include <stdarg.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#define END_POINT_ADDRESS_WRITE 0x01
#define END_POINT_ADDRESS_READ 0x81

#define READ_REG_COUNT_MAX 30
#define WRITE_REG_COUNT_MAX 28 

#define MB_FUNC_READ_INPUT_REGISTER 0x04
#define MB_FUNC_READ_HOLDING_REGISTER 0x03
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS 0x10

#define HID_PACK_MAX		64
#define HID_PACK_CH		0
#define HID_PACK_LEN		1
#define HID_PACK_TYPE		2
#define HID_PACK_MODBUS		3
#define REPORT_ID		0

void error_exit(const char* msg, int rc, ...) {
        char buffer[2048];
        memset(buffer, 0, sizeof(buffer));

        va_list args;
        va_start(args, rc);
        vsprintf(buffer, msg, args);
        va_end(args);

        printf("%s, %d/%s\r\n", buffer, rc, libusb_error_name(rc));

        exit(1);
}

usb_device::usb_device(libusb_device* d) : 
	device(d),
	handle(0),
	is_open(false),
	timeout_ms(1000)
{
        int r = libusb_open(device, &handle);
 	is_open = handle && r == 0; 
}

usb_device::~usb_device() {
	if(handle)
		libusb_close(handle);
	handle = 0;
	is_open = false;
}

/* same as the library version, but automatically handles retry on timeout */
int usb_device::usb_data_transfer(unsigned char endpoint_address,
                                  char* data,
                                  int length,
                                  int* total_transferred)
{
        int r = 0;

        int temp_total = 0;
        if(!total_transferred)
                total_transferred = &temp_total;

        int bytes_transferred = 0;
        char* data_ptr = data;

        while(1) {
                int transferred = 0;

                printf("bytes already transferred: %d, length: %d\r\n",
                        bytes_transferred,
                        length);

                r = libusb_interrupt_transfer(
                                        handle,
                                        endpoint_address,
                                        (unsigned char *)data + bytes_transferred,
                                        length - bytes_transferred,
                                        &transferred,
                                        timeout_ms);

                *total_transferred += transferred;
                printf("op %d, transferred bytes: %d, last result: %d/%s\r\n",
                        endpoint_address,
                        *total_transferred,
                        r,
                        libusb_error_name(r));

                if(r == LIBUSB_ERROR_TIMEOUT) {
                        printf("retrying...\r\n");
                } else if(r != 0) {
                        error_exit("an error was encountered during data transfer", r);
                }

                if(*total_transferred >= length)
                        return 0;
        }
}

int usb_device::modbus_request(char func_code, char* input, char *output) {
       	char data [HID_PACK_MAX + 1 /* report-id */];
       	memset(data, 0, sizeof(data));

	data[HID_PACK_CH] = REPORT_ID;
       	data[HID_PACK_TYPE] = 0x30;
	data[HID_PACK_MODBUS] = func_code;

	switch(func_code) {
		case MB_FUNC_READ_INPUT_REGISTER:
		case MB_FUNC_READ_HOLDING_REGISTER:
			data[HID_PACK_LEN] = 7;
			break;
		case MB_FUNC_WRITE_MULTIPLE_REGISTERS:
			data[HID_PACK_LEN] = 7 + input[4] + 1;
			if(data[HID_PACK_LEN] > HID_PACK_MAX)
				return MB_ELEN;
			break;
	}

	for(int i = 0; i < data[HID_PACK_LEN] - 3; i++)
		data[HID_PACK_MODBUS + 1 + i] = input[i];

	// write trans...
       	int r = usb_data_transfer(END_POINT_ADDRESS_WRITE, data, sizeof(data));
	if(r == 0) {
		// receiving
		data[HID_PACK_CH] = REPORT_ID;
               	if(usb_data_transfer(END_POINT_ADDRESS_READ, (char *)&data, sizeof(data)) == 0) {
			if(data[HID_PACK_LEN] > HID_PACK_MAX)
				return -1;

			if(data[HID_PACK_MODBUS] == func_code) {
				switch(func_code) {
					case MB_FUNC_READ_INPUT_REGISTER:
					case MB_FUNC_READ_HOLDING_REGISTER:
						if((data[HID_PACK_LEN] != data[HID_PACK_MODBUS + 1] + 4) || (data[HID_PACK_LEN] & 0x01))
							return MB_ELEN;
						// primitive byte swap.
						for(int i = 0; i < data[HID_PACK_MODBUS + 1]; i += 2) {
							output[i] = data[HID_PACK_MODBUS + 2 + i + 1];
							output[i + 1] = data[HID_PACK_MODBUS + 2 + i];
						}
						break;
					case MB_FUNC_WRITE_MULTIPLE_REGISTERS:
						break;
				}
			}
               	}
	}

	return r;
}

int usb_device::read_request(char func_code, int base_addr, int num_registers, char* dest) {
	int r = 0;

	for(int i = 0; i < num_registers / READ_REG_COUNT_MAX; ++i) {
        	read_data_registers read_req(base_addr, num_registers);
		r = modbus_request(func_code, (char *)&read_req, dest);
		if(r != 0)
			return r;
		base_addr += READ_REG_COUNT_MAX;
		dest += (READ_REG_COUNT_MAX * 2);
	}

	if(num_registers % READ_REG_COUNT_MAX) {
        	read_data_registers read_req(base_addr, num_registers);
		read_req.quantity_to_read.low = num_registers % READ_REG_COUNT_MAX;

		int r = modbus_request(func_code, (char *)&read_req, dest);
		if(r != 0)
			return r;
	}

	return r;
}

// 0x04 - read input registers
int usb_device::get_device_only(struct device_only* output) {	
  	return read_request(MB_FUNC_READ_INPUT_REGISTER, 0x0000, sizeof(device_only) / 2, (char *)output); 
}

usb_device_ptr usb_device::first_charger(libusb_context* ctx, int vendor, int product) {
        libusb_device **devs;
        size_t cnt = libusb_get_device_list(ctx, &devs);

        libusb_device* found = 0;

        for(size_t index = 0; index < cnt && !found; ++index) {
                struct libusb_device_descriptor desc;
                int r = libusb_get_device_descriptor(devs[index], &desc);
                if (r >= 0) {
                        if(desc.idVendor == vendor &&
                           desc.idProduct == product) {
                                found = devs[index];
                        }
                }
        }

	usb_device_ptr p;
        if(found) {
		p = usb_device_ptr(new usb_device(found));
	}
	
        libusb_free_device_list(devs, 1 /* unref all elements please */);

        return p;
}
