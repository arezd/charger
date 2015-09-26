#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>

#include "usb/icharger_usb.h"
#include "usb/hotplug_adapter.h"
#include "usb/eventhandler.h"

#include "nzmqt/nzmqt.hpp"
#include "zmq/zmq_publisher.h"

#include "bonjour/bonjourserviceregister.h"
#include "device_registry.h"
#include "message_handler.h"

namespace nzmqt {
    class ZMQContext;
}

class Controller : public QObject
{
    Q_OBJECT
public:
    explicit Controller(QObject *parent = 0);
    virtual ~Controller();
    
    int init();
signals:
    
public slots:
    void register_pub_port(int new_port);
    void register_msg_port(int new_port);
    
    void notify_hotplug_event(bool added, int vendor, int product, QString sn);
    void device_added(QString key);
    void device_removed(QString key);
    
private:
    usb_context _usb;
    nzmqt::ZMQContext* _ctx;
    ZMQ_Publisher* _pub;
    
    // ensures we publish services via bonjour/zeroconf
    BonjourServiceRegister* _bon;
    
    // handles usb hotplug events - see also notify_hotplug_event
    HotplugEventAdapter* _hotplug;
    UseQtEventDriver* _hotplug_handler;
    QThread* _hotplug_thread;
    
    // the internal device model - its really just a collection of stuff that can 
    // then be sent over the wire to listeners.  Events update this model and we
    // then push parts or all of it over the wire via ZMQ in JSON
    DeviceRegistry* _registry;
    
    // the message handler talks with GUI processes 
    MessageHandler* _msg_handler;
};

#endif // CONTROLLER_H
