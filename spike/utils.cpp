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

#define BASE_ADDR_DEVICE_ONLY 0x0000
#define BASE_ADDR_CHANNEL_STATUS1 0x0100
#define BASE_ADDR_CHANNEL_STATUS2 0x0200
#define BASE_ADDR_SYSTEM_STORAGE 0x8400

#define HID_PACK_MAX		64
#define HID_PACK_LEN		0
#define HID_PACK_TYPE		1
#define HID_PACK_MODBUS		2

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

icharger_usb::icharger_usb(libusb_device* d) : 
	device(d),
	handle(0),
	timeout_ms(1000)
{
        int r = libusb_open(device, &handle);
 	handle && r == 0; 
}

icharger_usb::~icharger_usb() {
	if(handle)
		libusb_close(handle);
	handle = 0;
}

void dump_ascii_hex(char* data, int len) {
	printf("from addr: %d for %d bytes\r\n", data, len);
	for(int i = 0; i < len; ++i) {
		printf("%2d: %2x %c\r\n", i, data[i], data[i]);
	}
	printf("----\r\n");

}

/* same as the library version, but automatically handles retry on timeout */
int icharger_usb::usb_data_transfer(unsigned char endpoint_address,
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

	//printf("initiate usb transfer to/from endpoint: %d, for total of %d bytes\r\n", endpoint_address, length);

        while(1) {
                int transferred = 0;

                r = libusb_interrupt_transfer(
                                        handle,
                                        endpoint_address,
                                        (unsigned char *)data + bytes_transferred,
                                        length - bytes_transferred,
                                        &transferred,
                                        timeout_ms);

                *total_transferred += transferred;
                /*printf("op %d, transferred bytes: %d, last result: %d/%s\r\n",
                        endpoint_address,
                        *total_transferred,
                        r,
                        libusb_error_name(r));*/

                if(r == LIBUSB_ERROR_TIMEOUT) {
                        //printf("retrying...\r\n");
                } else if(r != 0) {
                        error_exit("an error was encountered during data transfer", r);
                }

                if(*total_transferred >= length)
                        return 0;
        }
}

int icharger_usb::modbus_request(char func_code, char* input, char *output) {
       	char data [HID_PACK_MAX];
       	memset(data, 0, sizeof(data));

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

	//dump_ascii_hex(data, data[HID_PACK_LEN]);

	// write trans...
       	int r = usb_data_transfer(END_POINT_ADDRESS_WRITE, data, sizeof(data));
	if(r == 0) {
		//data[HID_PACK_CH] = REPORT_ID;
		memset(data, 0, sizeof(data));

               	if(usb_data_transfer(END_POINT_ADDRESS_READ, (char *)&data, sizeof(data)) == 0) {
			//printf("decoding returned data... data len: %d, modbus return is: %d\r\n", data[HID_PACK_LEN], data[HID_PACK_MODBUS]);

			if(data[HID_PACK_LEN] > HID_PACK_MAX) {
				//printf("data length wrong - its greater than max pack len\r\n");	
				// TODO: work out which error code to return.
				return -1;
			}

			//dump_ascii_hex(data, data[HID_PACK_LEN]);	

			if(data[HID_PACK_MODBUS] == func_code) {
				switch(func_code) {
					case MB_FUNC_READ_INPUT_REGISTER:
					case MB_FUNC_READ_HOLDING_REGISTER:
						if((data[HID_PACK_LEN] != data[HID_PACK_MODBUS + 1] + 4) || (data[HID_PACK_LEN] & 0x01)) {
							return MB_ELEN;
						}

						// primitive byte swap.
						for(int i = 0; i < data[HID_PACK_MODBUS + 1]; i += 2) {
							output[i] = data[HID_PACK_MODBUS+2+i+1];
							output[i + 1] = data[HID_PACK_MODBUS+2+i];
						}

						break;
					case MB_FUNC_WRITE_MULTIPLE_REGISTERS:
						break;
				}
			}

			if(data[HID_PACK_MODBUS] == (func_code | 0x80))
				return (eMBErrorCode) data[HID_PACK_MODBUS + 1];	
               	}
	}

	return r;
}

int icharger_usb::read_request(char func_code, int base_addr, int num_registers, char* dest) {
	int r = 0;

	//printf("read request from base_addr: %x, for %d registers, sizeof read_req: %d\r\n", base_addr, num_registers, sizeof(read_data_registers));

	for(int i = 0; i < num_registers / READ_REG_COUNT_MAX; ++i) {
        	read_data_registers read_req(base_addr, READ_REG_COUNT_MAX);
		r = modbus_request(func_code, (char *)&read_req, dest);
		if(r != 0)
			return r;

		base_addr += READ_REG_COUNT_MAX;
		dest += (READ_REG_COUNT_MAX * 2);
	}

	if(num_registers % READ_REG_COUNT_MAX) {
        	read_data_registers read_req(base_addr, num_registers);
		read_req.quantity_to_read.high = 0;
		read_req.quantity_to_read.low = num_registers % READ_REG_COUNT_MAX;

		//printf("i'm going to read this many regs: %d\r\n", read_req.quantity_to_read.low);

		int r = modbus_request(func_code, (char *)&read_req, dest);
		if(r != 0)
			return r;
	}

	return r;
}

int icharger_usb::write_request(int base_addr, int num_registers, char *input) {
	int r;
	char data[80];
	for(int i = 0; i < num_registers / WRITE_REG_COUNT_MAX; i++) {
		data[0] = (base_addr >> 8);
		data[1] = (base_addr & 0xff);
		data[2] = 0;
		data[3] = WRITE_REG_COUNT_MAX;
		data[4] = 2*WRITE_REG_COUNT_MAX;

		for(int j = 0; j < data[4]; j += 2) {
			data[5+j] = input[j+1];
			data[5+j+1] = input[j];
		}

		r = modbus_request(MB_FUNC_WRITE_MULTIPLE_REGISTERS, data, NULL);
		if(ret != 0)
			return ret;
	
		RegStart += WRITE_REG_COUNT_MAX;
		pIn += (2*WRITE_REG_COUNT_MAX);
	}

	if(RegCount%WRITE_REG_COUNT_MAX)
	{
		data[0] = (BYTE)(RegStart >> 8);
		data[1] = (BYTE)(RegStart & 0xff);
		data[2] = 0;
		data[3] = (BYTE) RegCount%WRITE_REG_COUNT_MAX;
		data[4] = 2*data[3];
		for(j=0;j<data[4];j=j+2)
		{
			data[5+j] = pIn[j+1];
			data[5+j+1] = pIn[j];
		}
		ret = MasterModBus(MB_FUNC_WRITE_MULTIPLE_REGISTERS,data,NULL,TIME_OUT);
		if(ret != MB_EOK)return ret;
	}
	return ret;
}
}

// 0x04 - read input registers at base address 0x0000
int icharger_usb::get_device_only(device_only* output) {	
  	return read_request(MB_FUNC_READ_INPUT_REGISTER, BASE_ADDR_DEVICE_ONLY, sizeof(device_only) / 2, (char *)output); 
}

int icharger_usb::get_channel_status(int channel /* 0 or 1 */, channel_status* output) {
	int addr = 0;

	if(channel == 0)
		addr = BASE_ADDR_CHANNEL_STATUS1;
	else
		addr = BASE_ADDR_CHANNEL_STATUS2;
	
	if(addr)
		return read_request(MB_FUNC_READ_INPUT_REGISTER, addr, sizeof(channel_status) / 2, (char *)output);
	
	return 1;
}

int icharger_usb::get_system_storage(system_storage* output) {
	device_only dev;
	int r = get_device_only(&dev);
	if(r == 0) {
		return read_request(MB_FUNC_READ_HOLDING_REGISTER, BASE_ADDR_SYSTEM_STORAGE, 
			sizeof(system_storage) / 2, (char *)output);
	}

	return r;	
}

icharger_usb_ptr icharger_usb::first_charger(libusb_context* ctx, int vendor, int product) {
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

	icharger_usb_ptr p;
        if(found) {
		p = icharger_usb_ptr(new icharger_usb(found));
	}
	
        libusb_free_device_list(devs, 1 /* unref all elements */);

        return p;
}
