#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <QObject>
#include <QByteArray>
#include <QList>

#include "zmq/socketbase.h"

/**
 * @brief The MessageHandler class is a zero-mq message router/dealer.  It receives requests to do stuff with the
 * attached chargers and uses the device registry as well as the publishing service to satisfy those tasks.  All
 * GUI apps talk to this sucker.
 * 
 * This class is also (indirectly) responsible for translating the underlying data structures into easier 
 * to consume JSON messages before they go onto the wire. 
 * @see device_only, channel_status, system_storage 
 */
class MessageHandler : public SocketBase
{
    Q_OBJECT
    
public:
    explicit MessageHandler(nzmqt::ZMQContext* ctx, QObject *parent = 0);
    bool bind(int port);    
    
    void sendResponse(QList<QByteArray> return_path, QList<QByteArray> payload);
    void sendResponse(QList<QByteArray> return_path, QByteArray payload);
    
signals:
    void requesetReceived(QList<QByteArray> return_path, QList<QByteArray> payload);
    
protected slots:
    void processMessageRequest(QList<QByteArray> msg);
};

#endif // MESSAGE_HANDLER_H
