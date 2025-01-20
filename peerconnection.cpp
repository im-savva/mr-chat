#include "qnetworkinterface.h"
#define MESSAGE_DELIMITER "###"

#include "peerconnection.h"
#include "chatcardstate.h"
#include "encryption.h"
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QEventLoop>
#include <QByteArray>
#include <QNetworkReply>
#include <QElapsedTimer>

PeerConnection::PeerConnection(QObject *parent, std::vector<chatCardState *> *chatCardStateObjects_) : QObject(parent) {
    chatCardStateObjects = chatCardStateObjects_;
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &PeerConnection::onNewConnection);
}

// QString PeerConnection::getCurrentIPAddress() {
//     QString externalIP;

//     QNetworkAccessManager manager;
//     QNetworkRequest request(QUrl("https://api.ipify.org?format=json"));

//     QNetworkReply *reply = manager.get(request);
//     QEventLoop loop;
//     QObject::connect(reply, &QNetworkReply::finished, [&]() {
//         if (reply->error() == QNetworkReply::NoError) {
//             QByteArray data = reply->readAll();
//             QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
//             QJsonObject jsonObject = jsonDoc.object();
//             externalIP = jsonObject.value("ip").toString();
//         } else {
//             qDebug() << "Error retrieving external IP address:" << reply->errorString();
//         }
//         loop.quit();
//         reply->deleteLater();
//     });
//     loop.exec();

//     return externalIP;
// }

QString PeerConnection::getCurrentIPAddress() {
    QList<QHostAddress> ipAddressesList;

    // Loop through all network interfaces
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        // Skip loopback interfaces and interfaces that are not up
        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack) || !iface.flags().testFlag(QNetworkInterface::IsUp))
            continue;

        QList<QNetworkAddressEntry> addresses = iface.addressEntries();
        for (const QNetworkAddressEntry &address : addresses) {
            // Check for IPv4 protocol
            if (address.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                if (!address.ip().isLoopback() && !address.ip().isLinkLocal()) {
                    ipAddressesList.append(address.ip());
                    // return address.ip().toString();
                } else {
                    ipAddressesList.append(address.ip());
                }
            }
        }
    }

    if (!ipAddressesList.isEmpty()) {
        return ipAddressesList.last().toString();
    }

    return QString();
}

bool PeerConnection::start(quint16 connectionPort_) {

    qDebug() << "Starting server.";

    connectionPort = connectionPort_;
    return server->listen(QHostAddress::Any, connectionPort);
}

void PeerConnection::connectChats() {
    if (chatCardStateObjects != nullptr) {
        for (chatCardState* chatCardStateObject : *chatCardStateObjects) {
            if (chatCardStateObject != nullptr && chatCardStateObject->chatId != "new" && chatCardStateObject->isOnline == false) {
                connectToPeer(chatCardStateObject->ipAddress, chatCardStateObject->port, chatCardStateObject->chatId);
            }
        }
    }
}

bool PeerConnection::stop() {

    qDebug() << "Stopping server.";

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

void PeerConnection::connectToPeer(const QString &peerAddress, const quint16& peerPort, const QString &chatId) {
    for (QTcpSocket *socket : m_sockets) {
        if (socket->property("chatId").toString() == chatId) {
            qDebug() << "Already connected to " << peerAddress << ":" << peerPort << " (" << chatId << ")";
            return;
        }
    }

    QTcpSocket *socket = new QTcpSocket(server);

    connect(socket, &QTcpSocket::connected, this, &PeerConnection::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &PeerConnection::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &PeerConnection::onDisconnected);
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
    socket->setProperty("isAuthenticated", "false");
    socket->setProperty("chatId", chatId);
    socket->connectToHost(peerAddress, peerPort);
    m_sockets.append(socket);
    qDebug() << "Connecting to " << peerAddress << ":" << peerPort << " (" << chatId << ")";
}

void PeerConnection::disconnectFromPeer(const QString &chatId) {
    for (QTcpSocket *socket : m_sockets) {
        if (socket->property("chatId").toString() == chatId) {
            qDebug() << "Disconnecting from " << socket->peerAddress().toString() << ":" << QString::number(socket->peerPort()) << " (" << socket->property("chatId").toString() << ")";
            socket->disconnect(this);
            socket->disconnectFromHost();
            m_sockets.removeAll(socket);
        }
    }
}

void PeerConnection::sendMessage(const QJsonObject &jsonObject, const QString &chatId) {
    messageQueue.push_back(std::make_pair(jsonObject, chatId));
    if (messageQueue.size() == 1) {
        proccessMessagesQueue();
    }
}

void PeerConnection::proccessMessagesQueue() {
    qDebug() << messageQueue;
    if (!messageQueue.empty()) {
        const auto& messagePair = messageQueue.front();
        const QJsonObject& jsonObject = messagePair.first;
        const QString& chatId = messagePair.second;

        QJsonDocument jsonDoc(jsonObject);
        QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);
        jsonData.append(MESSAGE_DELIMITER);

        bool foundSocket = false;

        for (QTcpSocket *socket : m_sockets) {
            if (socket->property("chatId").toString() == chatId) {
                foundSocket = true;

                const int MAX_CHUNK_SIZE = 1024;
                int totalBytesToSend = jsonData.size();

                for (int i = 0; i < jsonData.size(); i += MAX_CHUNK_SIZE) {
                    int chunkSize = qMin(MAX_CHUNK_SIZE, jsonData.size() - i);
                    QByteArray chunkData = jsonData.mid(i, chunkSize);
                    qint64 bytesWritten = socket->write(jsonData);

                    if (bytesWritten == -1) {
                        qDebug() << "Error writing data to socket:" << socket->errorString();
                        messageQueue.erase(messageQueue.begin());
                        proccessMessagesQueue();
                    } else if (bytesWritten == totalBytesToSend) {
                        qDebug() << "Sending to " << socket->peerAddress().toString() << ":" << QString::number(socket->peerPort()) << " (" << socket->property("chatId").toString() << ")";
                        // qDebug() << jsonObject;
                        auto handleBytesWritten = [=]() {
                            QObject::disconnect(socket, &QTcpSocket::bytesWritten, nullptr, nullptr);
                            if (chatId == "initial-connection-response") {
                                emit initialConnectionResponseSent();
                            } else if (socket->property("isAuthenticated") == "false" && jsonObject["type"] == "authentication") {
                                socket->setProperty("isAuthenticated", "true");
                                emit onChatConnectedEvent(socket->property("chatId").toString());
                            } else if (jsonObject["type"] == "message-state") {
                                emit messageStateSent(jsonDoc);
                            } else if (jsonObject["type"] == "message-sync") {
                                emit messageSyncSent(jsonDoc);
                            } else if (jsonObject["type"] == "message") {
                                emit messageSent(jsonDoc);
                            }
                        };
                        QObject::connect(socket, &QTcpSocket::bytesWritten, this, handleBytesWritten);
                    }
                }
                messageQueue.erase(messageQueue.begin());
                proccessMessagesQueue();
                break;
            }
        }

        if (!foundSocket) {
            qDebug() << "SOCKET NOT FOUND";
            messageQueue.erase(messageQueue.begin());
            proccessMessagesQueue();
        }
    }
}

void PeerConnection::onNewConnection() {
    QTcpSocket *clientSocket = server->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &PeerConnection::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &PeerConnection::onDisconnected);
    m_sockets.append(clientSocket);

    qDebug() << "New anonymous connection.";
}

void PeerConnection::onConnected() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;
    qDebug() << "Connected to " << socket->peerAddress().toString() << ":" << QString::number(socket->peerPort()) << " (" << socket->property("chatId").toString() << ")";

    QString chatId = socket->property("chatId").toString();
    if (chatId == "initial-connection-response") {
        emit initialConnectionResponseConnected();
    }
    else if (socket->property("isAuthenticated") == "false" && chatCardStateObjects != nullptr) {
        for (chatCardState* chatCard : *chatCardStateObjects) {
            if (chatCard->chatId == chatId) {
                // Send my credentials to authenticate
                AES_KEY *chatAesKey = chatCard->chatAesKey;
                QByteArray iv = generateRandomIV();
                QJsonObject message;
                message["type"] = "authentication";
                QJsonObject credentials;
                credentials["chatId"] = chatId;
                QString ivString = iv.toBase64();
                credentials["iv"] = ivString;
                message["credentials"] = credentials;
                QString ipAddress = getCurrentIPAddress();
                QJsonObject data;
                data["ipAddress"] = ipAddress;
                data["port"] = connectionPort;
                QJsonDocument dataJsonDoc(data);
                QByteArray encryptedData = encryptQStringAES(dataJsonDoc.toJson(QJsonDocument::Compact), chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
                QString dataString = encryptedData.toBase64();
                message["data"] = dataString;
                sendMessage(message, chatId);
            }
        }
    }
}

void PeerConnection::onDisconnected() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;
    qDebug() << "Disconnected from " << socket->peerAddress().toString() << ":" << QString::number(socket->peerPort()) << " (" << socket->property("chatId").toString() << ")";

    m_sockets.removeAll(socket);

    emit onChatDisconnectedEvent(socket->property("chatId").toString());
}

bool isProccessingBuffer = false;

void PeerConnection::onReadyRead() {
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket)
        return;

    QHostAddress peerAddress = senderSocket->peerAddress();
    quint16 peerPort = senderSocket->peerPort();
    QString peerIpAddress = QHostAddress(peerAddress.toIPv4Address()).toString();
    QString peerChatId = "anonymous";
    if (!senderSocket->property("chatId").isNull()) {
        peerChatId = senderSocket->property("chatId").toString();
    }

    QByteArray data = senderSocket->readAll();
    receivingMessagesBuffer.append(data);

    int delimiterIndex;

    if (!isProccessingBuffer) {
        isProccessingBuffer = true;
        while ((delimiterIndex = receivingMessagesBuffer.indexOf(MESSAGE_DELIMITER)) != -1) {
            QByteArray jsonData = receivingMessagesBuffer.left(delimiterIndex);
            receivingMessagesBuffer.remove(0, delimiterIndex + 3);

            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);

            qDebug() << "Message from " << peerIpAddress << ":" << QString::number(peerPort) << " (" << peerChatId << ")";

            if (!jsonDoc.isObject()) {
                qDebug() << "Received data is not in JSON format.";
            }
            if (jsonDoc["type"] == "initial-connection-request") {
                emit initialConnectionRequest(jsonDoc);
            } else if (jsonDoc["type"] == "authentication" && !chatCardStateObjects->empty() && chatCardStateObjects != nullptr) {

                qDebug() << "TRYING TO AUTHENTICATE";

                QString chatId = jsonDoc["credentials"]["chatId"].toString();
                QByteArray iv = QByteArray::fromBase64(jsonDoc["credentials"]["iv"].toString().toUtf8());
                QByteArray encryptedCredentials = QByteArray::fromBase64(jsonDoc["data"].toString().toUtf8());

                bool wasChatFound = false;

                for (chatCardState* chatCard : *chatCardStateObjects) {
                    if (chatCard != nullptr && chatCard->chatId == chatId) {
                        wasChatFound = true;
                        AES_KEY* chatAesKey = chatCard->chatAesKey;
                        QString decryptedCredentials = decryptQStringAES(encryptedCredentials, chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
                        QJsonDocument credentialsJson = QJsonDocument::fromJson(decryptedCredentials.toUtf8());
                        if (credentialsJson["ipAddress"].toString() == chatCard->ipAddress && credentialsJson["port"].toInt() == chatCard->port) {
                            for (QTcpSocket *socket : m_sockets) {
                                if (socket->property("chatId").toString() == chatId) {
                                    m_sockets.removeAll(socket);
                                    break;
                                }
                            }
                            senderSocket->setProperty("chatId", chatCard->chatId);
                            emit onChatConnectedEvent(chatCard->chatId);
                        }
                        break;
                    }
                }

                if (!wasChatFound) {
                    qDebug() << "Authentication failed.";
                    senderSocket->disconnect(this);
                    senderSocket->disconnectFromHost();
                    m_sockets.removeAll(senderSocket);
                } else {
                    qDebug() << "Authentication successfull.";
                }

            } else if (jsonDoc["type"] == "authentication") {
                qDebug() << "Authentication failed.";
                senderSocket->disconnect(this);
                senderSocket->disconnectFromHost();
                m_sockets.removeAll(senderSocket);
            } else if (jsonDoc["type"] == "message") {
                emit messageReceiving(jsonDoc);
            } else if (jsonDoc["type"] == "message-sync") {
                emit messageSyncReceiving(jsonDoc);
            } else if (jsonDoc["type"] == "message-state") {
                emit messageStateSent(jsonDoc);
            } else if (jsonDoc["type"] == "chat-ban") {
                emit messageStateSent(jsonDoc);
            } else {
                qDebug() << "Unknown message type";
            }
        }
        isProccessingBuffer = false;
    }
}

void PeerConnection::onError(QAbstractSocket::SocketError socketError) {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    qDebug() << "Socket error:" << socket->errorString();
    m_sockets.removeOne(socket);
    socket->deleteLater();
}
