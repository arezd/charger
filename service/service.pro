
TEMPLATE = app
QT += network
QT -= gui

CONFIG += console
CONFIG -= app_bundle
CONFIG += debug

SOURCES = \
    main.cpp \
    usb/icharger_usb.cpp \
    bonjour/bonjourserviceregister.cpp \
    zmq/zmq_publisher.cpp \
    usb/eventhandler.cpp \
    usb/hotplug_adapter.cpp \
    controller.cpp 

HEADERS = \ 
    bonjour\bonjourrecord.h \
    usb/icharger_usb.h \
    usb/icharger_data.h \
    util/singleton.h \
    bonjour/bonjourrecord.h \
    bonjour/bonjourserviceregister.h \
    zmq/zmq_publisher.h \
    usb/eventhandler.h \
    usb/hotplug_adapter.h \
    controller.h 

unix:!macx {
    INCLUDEPATH += ../3rdparty/build/include/libusb-1.0
    LIBS += ../3rdparty/build/lib/libusb-1.0.a -L/usr/lib/arm-linux-gnueabihf -lzmq -ldns_sd -lrt -ludev
}

macx {
    INCLUDEPATH += /usr/local/include ../3rdparty/build/include/libusb-1.0
    LIBS += -L/usr/local/lib -lusb-1.0 -lzmq
}

TARGET = service

include (../3rdparty/nzmqt/nzmqt.pri)



