#include <QDebug>
#include <QList>
#include <QByteArray>
#include <QStringList>

#include "zmq/publisher.h"
#include "json_helper.h"

using namespace nzmqt;

Publisher::Publisher(ZMQContext* ctx, QObject* owner) : SocketBase(ctx, ZMQSocket::TYP_PUB, owner) {
}

bool Publisher::bind(int port) {
    QString port_value("*");
    if(port != 0)
        port_value = QString::number(port);
    _socket->bindTo(QString("tcp://*:%1").arg(port_value));
    
    char temp[255];
    size_t value_len = 255;
    _socket->getOption(ZMQSocket::OPT_LAST_ENDPOINT, temp, &value_len);
    
    QStringList parts = QString::fromLatin1(temp).split(":");
    setPort(parts.last().toInt());
    
    return _port != 0;
}

void Publisher::publishHeartbeat() {
    publishOnTopic("/heartbeat", "ping");
}

void Publisher::publishDeviceAddedRemoved(bool added, QString key) {
    QVariantMap data;
    data["added"] = added;
    data["key"] = key;
    publishOnTopic("/device", variantMapToJson(data));    
}

void Publisher::publishOnTopic(QByteArray topic, QByteArray content) {
    QList< QByteArray > msg;
    msg += topic;
    msg += content;
    _socket->sendMessage(msg);
}




