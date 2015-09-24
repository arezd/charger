
TEMPLATE = app
QT += network
QT -= gui

CONFIG += c++11
CONFIG += console
CONFIG -= app_bundle

SOURCES = \
    main.cpp \
    usb/icharger_usb.cpp \
    bonjour/bonjourservicebrowser.cpp \
    bonjour/bonjourserviceregister.cpp \
    bonjour/bonjourserviceresolver.cpp \
    zmq/zmq_publisher.cpp \
    usb/usb_hotpluglistener.cpp \
    usb/eventhandler.cpp

unix:INCLUDEPATH += /usr/include/libusb-1.0
unix:LIBS += -L/usr/lib -lusb-1.0

macx:INCLUDEPATH += /usr/local/include /usr/local/include/libusb-1.0
macx:LIBS += -L/usr/local/lib -lusb-1.0 -zmq

HEADERS = \ 
    bonjour\bonjourrecord.h \
    usb/icharger_usb.h \
    usb/icharger_data.h \
    util/singleton.h \
    bonjour/bonjourrecord.h \
    bonjour/bonjourservicebrowser.h \
    bonjour/bonjourserviceregister.h \
    bonjour/bonjourserviceresolver.h \
    zmq/zmq_publisher.h \
    usb/usb_hotpluglistener.h \
    usb/eventhandler.h

TARGET = service

include (../3rdparty/nzmqt/nzmqt.pri)



