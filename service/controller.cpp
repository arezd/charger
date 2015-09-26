#include "controller.h"

#include <QtGlobal>
#include <QDebug>
#include <QThread>
#include <QString>
#include <QStringList>
#include <QScopedPointer>
#include <QDebug>

#include "usb/icharger_usb.h"
#include "usb/hotplug_adapter.h"
#include "usb/eventhandler.h"

#include "nzmqt/nzmqt.hpp"
#include "zmq/zmq_publisher.h"

#include "bonjour/bonjourserviceregister.h"

//Q_LOGGING_CATEGORY(controller, "controller")

using namespace nzmqt;
using namespace std;

Controller::Controller(QObject *parent) : QObject(parent), 
    _ctx(0),
    _pub(0), 
    _hotplug(0),
    _hotplug_handler(0),
    _hotplug_thread(0),
    _registry(0)
{
}

Controller::~Controller() {
    delete _registry;
    _registry = 0;
    
    _hotplug_handler->killTimer(_hotplug_handler->timer);
    _hotplug_handler->deleteLater();
    _hotplug_handler = 0;
    
    _hotplug_thread->quit();
    _hotplug_thread->deleteLater();    
    _hotplug_thread = 0;
    
    delete _pub;
    _pub = 0;
    
    delete _bon;
    _bon = 0;
    
    _ctx->stop();
    delete _ctx;
    _ctx = 0;
}

int Controller::init() {
    try {
        // ZeroMQ to send / receive messages - requires a context.
        _ctx = createDefaultContext(this);
        _ctx->start();
        
        // Get the message infrastructure ready (pub/sub and messaging).
        _pub = new ZMQ_Publisher(_ctx);
        
        // Bonjour system needs to publish our ZMQ publisher port
        _bon = new BonjourServiceRegister;
        
        // Respond to changes to the publisher port
        connect(_pub, SIGNAL(port_changed(int)), 
                this, SLOT(register_pub_port(int)));
        
        // TODO: Respond to changes to the messaging port. 
        
        // bind the publisher to cause it to find a local ephemeral port and publish it
        if(!_pub->bind()) {
            qDebug() << "unable to bind the zmq publisher to its interface";
            return 1;
        }
        
        // Kick off a listener for USB hotplug events - so we keep our device list fresh
        _hotplug = new HotplugEventAdapter();
        QObject::connect(_hotplug, SIGNAL(hotplug_event(bool, int, int, QString)), 
                         this, SLOT(notify_hotplug_event(bool, int, int, QString)));
        
        // Primitive libsusb event handler.
        _hotplug_handler = new UseQtEventDriver(_usb.ctx);
        _hotplug_thread = new QThread();
        _hotplug_handler->moveToThread(_hotplug_thread);
        _hotplug_thread->start();
        
        _hotplug->init(_usb.ctx);
     
        _registry = new DeviceRegistry(_usb.ctx);
        
        QObject::connect(_registry, SIGNAL(device_activated(QString)),
                         this, SLOT(device_added(QString)));
        QObject::connect(_registry, SIGNAL(device_deactivated(QString)),
                         this, SLOT(device_removed(QString)));
        
        return 0;
    }
    
    catch(zmq::error_t& ex) {
        qDebug()<< "failed to init zmq:" << ex.what();
        return 1;
    }
}

void Controller::register_pub_port(int new_port) {
    _bon->registerService("_charger-service-pub._tcp", new_port);   
    qDebug() << "pub/sub comms are being made on port number:" << new_port;
}

void Controller::notify_hotplug_event(bool added, int vendor, int product, QString sn)  {
    qDebug() << "hotplug event for vendor:" << vendor << ", product:" << product << ", serial number:" << sn << ", registry:" << _registry;
    if(added)
        _registry->activate_device(vendor, product, sn);
    else
        _registry->deactivate_device(vendor, product);
}

void Controller::device_added(QString key) {
    qDebug() << "a device was added to the registry:" << key;
}

void Controller::device_removed(QString key) {
    qDebug() << "a device was removed from the registry:" << key;
}
