#define MESSAGE_DELIMITER "###"

#include "peerinitialconnection.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QElapsedTimer>

PeerInitialConnection::PeerInitialConnection(QObject *parent) : QObject(parent) {
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &PeerInitialConnection::onNewConnection);
}

bool PeerInitialConnection::start(const quint16 *connectionPort_) {
    qDebug() << "Starting initial connection server.";
    connectionPort = connectionPort_;
    return server->listen(QHostAddress::Any, *connectionPort);
}

bool PeerInitialConnection::stop() {

    qDebug() << "Stopping initial connection server.";

    server->close();

    if (m_sockets.isEmpty()) {
        return true;
    }

    for (QTcpSocket *socket : m_sockets) {
        socket->disconnectFromHost();
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->waitForDisconnected();
        }
        socket->deleteLater();
    }
    m_sockets.clear();

    return true;
}

void PeerInitialConnection::connectToPeer(const QString &peerAddress, const quint16& peerPort) {

    QTcpSocket *socket = new QTcpSocket(server);

    connect(socket, &QTcpSocket::readyRead, this, &PeerInitialConnection::onReadyRead);
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
    socket->setProperty("chatId", "initial-connection-response");
    socket->connectToHost(peerAddress, peerPort);
    m_sockets.append(socket);
}

bool PeerInitialConnection::sendConnectionMessage(const QJsonObject &jsonObject) {
    for (QTcpSocket *socket : m_sockets) {
        if (socket->property("chatId").toString() == "initial-connection-response") {
            QJsonDocument jsonDoc(jsonObject);
            QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);
            jsonData.append(MESSAGE_DELIMITER);

            if (!socket->waitForConnected(30000)) {
                return false;
            }

            qint64 bytesWritten = socket->write(jsonData);
            if (bytesWritten == -1) {
                return false;
            }

            if (!socket->waitForBytesWritten(30000)) {
                return false;
            }

            return true;
        }
    }
    return false;
}

void PeerInitialConnection::onNewConnection() {
    QTcpSocket *clientSocket = server->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &PeerInitialConnection::onReadyRead);
    m_sockets.append(clientSocket);
}

void PeerInitialConnection::onReadyRead() {
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket)
        return;

    QByteArray jsonData = senderSocket->readAll();
    jsonData = jsonData.left(jsonData.size() - 3);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    qDebug() << jsonDoc;

    if (!jsonDoc.isObject()) {
        qDebug() << "Received data is not in JSON format.";
    }

    qDebug() << jsonDoc["type"];
    qDebug() << (jsonDoc["type"] == "initial-connection-response");

    if (jsonDoc["type"] == "initial-connection-response") {
        qDebug() << "Recieved connection response";
        stop();
        emit initialConnectionResponse(jsonDoc);
    } else {
        qDebug() << "Unknown message type";
    }
}

void PeerInitialConnection::onError(QAbstractSocket::SocketError socketError) {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    qDebug() << "Socket error:" << socket->errorString();
    m_sockets.removeOne(socket);
    socket->deleteLater();
}
