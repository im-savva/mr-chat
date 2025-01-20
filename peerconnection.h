#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include "chatcardstate.h"
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <queue>

class PeerConnection : public QObject
{
    Q_OBJECT
public:
    explicit PeerConnection(QObject *parent = nullptr, std::vector<chatCardState*>* chatCardStateObjects_ = nullptr);
    QString getCurrentIPAddress();
    bool start(quint16 connectionPort_);
    bool stop();
    void connectToPeer(const QString &peerAddress, const quint16& peerPort, const QString &chatId);
    void disconnectFromPeer(const QString &chatId);
    void sendMessage(const QJsonObject &jsonObject, const QString &chatId);
    void proccessMessagesQueue();
    void connectChats();

private slots:
    void onNewConnection();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

signals:
    void initialConnectionRequest(const QJsonDocument& connectionDetails);
    void initialConnectionResponse(const QJsonDocument& connectionDetails);
    void messageReceiving(const QJsonDocument& messageDetails);
    void messageSyncReceiving(const QJsonDocument& messageDetails);
    void messageStateSent(const QJsonDocument& messageDetails);
    void messageSent(const QJsonDocument& messageDetails);
    void messageSyncSent(const QJsonDocument& messageDetails);
    void onChatConnectedEvent(const QString& chatId);
    void onChatDisconnectedEvent(const QString& chatId);
    void initialConnectionResponseSent();
    void initialConnectionResponseConnected();

private:
    std::vector<std::pair<QJsonObject, QString>> messageQueue;
    std::vector<chatCardState *> *chatCardStateObjects;
    QTcpServer *server;
    QList<QTcpSocket*> m_sockets;
    quint16 connectionPort;
    QByteArray receivingMessagesBuffer;
};

#endif // PEERCONNECTION_H
