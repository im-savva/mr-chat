#ifndef PEERINITIALCONNECTION_H
#define PEERINITIALCONNECTION_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class PeerInitialConnection : public QObject
{
    Q_OBJECT
public:
    explicit PeerInitialConnection(QObject *parent = nullptr);
    QString getCurrentIPAddress();
    bool start(const quint16* connectionPort_);
    bool stop();
    void connectToPeer(const QString &peerAddress, const quint16& peerPort);
    bool sendConnectionMessage(const QJsonObject &jsonObject);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

signals:
    void initialConnectionResponse(const QJsonDocument& connectionDetails);

private:
    QTcpServer *server;
    QList<QTcpSocket*> m_sockets;
    const quint16 *connectionPort;
};

#endif // PEERINITIALCONNECTION_H
