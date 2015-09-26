#ifndef USB_HOTPLUGLISTENER_H
#define USB_HOTPLUGLISTENER_H

#include <QObject>

/**
 * @brief The HotplugEventAdapter class is responsible to matching hotplug events to the USB driver code 
 * for each known device.  When a hotplug event is detected for a device we know about, the serial number
 * of the devic is obtained and a signal is fired indicating whether the device was attached or removed.
 * 
 * The serial number is only obtained for attachment events.
 */
class HotplugEventAdapter : public QObject
{
    Q_OBJECT
public:
    explicit HotplugEventAdapter(QObject *parent = 0);
    virtual ~HotplugEventAdapter();

    void init(void *ctx);    
    void process_hotplug_event(int event_type, int vendor, int product, QString sn);
    
signals:
    void hotplug_event(bool device_arrived, int vendor, int product, QString sn);
    
public slots:
    
private:
    struct Private;
    Private* _p;
};

#endif // USB_HOTPLUGLISTENER_H
