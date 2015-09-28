QT += core
QT -= gui
QT += network

TEMPLATE = app
TARGET = tester
CONFIG += console
CONFIG -= app_bundle

INCLUDEPATH += ../service

unix:!macx {
    INCLUDEPATH += ../3rdparty/build/include/libusb-1.0
    LIBS += ../3rdparty/build/lib/libusb-1.0.a -L/usr/lib/arm-linux-gnueabihf -lzmq -ldns_sd -lrt -ludev
}

macx {
    INCLUDEPATH += /usr/local/include /usr/local/include/libusb-1.0
    LIBS += -L/usr/local/lib -lusb-1.0 -lzmq 
}

HEADERS += \
    ../service/bonjour/bonjourrecord.h \
    ../service/bonjour/bonjourservicebrowser.h \
    ../service/bonjour/bonjourserviceresolver.h \
    registeredtyperesolver.h \
    client_controller.h \
    message_bus.h

SOURCES += main.cpp \
    ../service/bonjour/bonjourservicebrowser.cpp \
    ../service/bonjour/bonjourserviceresolver.cpp \
    registeredtyperesolver.cpp \
    client_controller.cpp \
    message_bus.cpp

include (../3rdparty/nzmqt/nzmqt.pri)

