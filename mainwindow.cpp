#include "mainwindow.h"
#include "accountsettings.h"
#include "chatsettings.h"
#include "cmath"
#include <QQueue>
#include <QJsonArray>
#include <QTimeZone>
#include "accountchoice.h"
#include "addaccount.h"
#include "emojispicker.h"
#include "filemessage.h"
#include "addchat.h"
#include "addchatprogress.h"
#include "addchatrequest.h"
#include "chatcard.h"
#include "chatcardstate.h"
#include "loginaccount.h"
#include <QPropertyAnimation>
#include "messagestate.h"
#include "qcryptographichash.h"
#include "qevent.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "qsqlerror.h"
#include "qsqlquery.h"
#include "ui_mainwindow.h"
#include <QSizePolicy>
#include <QScrollBar>
#include <QTimer>
#include <QNetworkInterface>
#include <QDir>
#include <QFileDialog>
#include "encryption.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    messagesListVerticalScrollBar = ui->messageListContainer->verticalScrollBar();
    showAccountChoiceDialog();
    if (accountId.isEmpty()) {
        QMetaObject::invokeMethod(this, "closeApp", Qt::QueuedConnection);
    }
}

void MainWindow::closeApp() {
    close();
}

void MainWindow::proccessMessageReceiving(const QJsonDocument& messageDetails) {
    QString chatId = messageDetails["credentials"]["chatId"].toString();
    QByteArray iv = QByteArray::fromBase64(messageDetails["credentials"]["iv"].toString().toUtf8());
    QByteArray encryptedData = QByteArray::fromBase64(messageDetails["data"].toString().toUtf8());

    for (chatCardState* chatCard : chatCardStateObjects) {
        if (chatCard != nullptr && chatCard->chatId == chatId) {
            AES_KEY* chatAesKey = chatCard->chatAesKey;
            QString decryptedDataString = decryptQStringAES(encryptedData, chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
            QJsonDocument decryptedDataJson = QJsonDocument::fromJson(decryptedDataString.toUtf8());

            qDebug() << decryptedDataJson;
            if (decryptedDataJson["type"].toString() == "ban-status") {
                chatCard->setChatBanned(chatCard->isBanned, decryptedDataJson["isBanned"].toBool());
                if (openedChat == chatCard) {
                    QString prevOpenedChat = openedChat->chatId;
                    closeOpenedChat();
                    openChat(prevOpenedChat);
                }
            } else if (!chatCard->isBanned) {
                QString messageTimestampString = decryptedDataJson["timestamp"].toString();
                QDateTime messageTimestamp = QDateTime::fromString(messageTimestampString, Qt::ISODate);
                messageTimestamp.setTimeZone(QTimeZone("Europe/Moscow"));
                addMessage(chatId, decryptedDataJson["id"].toString(), decryptedDataJson["type"].toString(), messageTimestamp, messageTimestampString, decryptedDataJson["sender"].toString(), decryptedDataJson["value"].toObject(), decryptedDataJson["state"].toString());
            }

            break;
        }
    }
}

void MainWindow::proccessMessageSyncReceiving(const QJsonDocument& messagesDetails) {

    qDebug() << "SYNCING FROM COMPANION";

    QJsonArray messagesArray = messagesDetails["data"].toArray();

    for (const auto& messageDetails_: messagesArray) {
        QJsonObject messageDetails = messageDetails_.toObject();
        QJsonObject messageCredentials = messageDetails["credentials"].toObject();
        QString chatId = messageCredentials["chatId"].toString();
        QByteArray iv = QByteArray::fromBase64(messageCredentials["iv"].toString().toUtf8());
        QByteArray encryptedData = QByteArray::fromBase64(messageDetails["data"].toString().toUtf8());

        for (chatCardState* chatCard : chatCardStateObjects) {
            if (chatCard != nullptr && chatCard->chatId == chatId) {
                AES_KEY* chatAesKey = chatCard->chatAesKey;
                QString decryptedDataString = decryptQStringAES(encryptedData, chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
                QJsonDocument decryptedDataJson = QJsonDocument::fromJson(decryptedDataString.toUtf8());

                if (decryptedDataJson["type"].toString() == "ban-status") {
                    chatCard->setChatBanned(chatCard->isBanned, decryptedDataJson["isBanned"].toBool());
                    if (openedChat == chatCard) {
                        QString prevOpenedChat = openedChat->chatId;
                        closeOpenedChat();
                        openChat(prevOpenedChat);
                    }
                } else if (!chatCard->isBanned) {
                    QString messageTimestampString = decryptedDataJson["timestamp"].toString();
                    QDateTime messageTimestamp = QDateTime::fromString(messageTimestampString, Qt::ISODate);
                    messageTimestamp.setTimeZone(QTimeZone("Europe/Moscow"));

                    addMessage(chatId, decryptedDataJson["id"].toString(), decryptedDataJson["type"].toString(), messageTimestamp, messageTimestampString, decryptedDataJson["sender"].toString(), decryptedDataJson["value"].toObject(), decryptedDataJson["state"].toString(), false);
                }

                break;
            }
        }
    }


    if (openedChat != nullptr && messagesDetails["credentials"]["chatId"].toString() == openedChat->chatId) {
        renderMessagesList("", true);
    }
}

void MainWindow::proccessMessageSyncSent(const QJsonDocument& messagesDetails) {

    qDebug() << "SYNCING SENT";

    QJsonArray messagesArray = messagesDetails["data"].toArray();

    for (const auto& messageDetails_: messagesArray) {
        QJsonObject messageDetails = messageDetails_.toObject();
        QJsonObject messageCredentials = messageDetails["credentials"].toObject();
        QString chatId = messageCredentials["chatId"].toString();
        QByteArray iv = QByteArray::fromBase64(messageCredentials["iv"].toString().toUtf8());
        QByteArray encryptedData = QByteArray::fromBase64(messageDetails["data"].toString().toUtf8());

        for (chatCardState* chatCard : chatCardStateObjects) {
            if (chatCard != nullptr && chatCard->chatId == chatId) {

                AES_KEY* chatAesKey = chatCard->chatAesKey;
                QString decryptedDataString = decryptQStringAES(encryptedData, chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
                QJsonDocument decryptedDataJson = QJsonDocument::fromJson(decryptedDataString.toUtf8());

                qDebug() << "PROCCESS MESSAGE SENT" << decryptedDataJson["id"].toString();

                for (messageState* messageStateObject_ : messageStateObjects) {
                    if (messageStateObject_ != nullptr && messageStateObject_->messageId == decryptedDataJson["id"].toString()) {
                        messageStateObject_->updateState("sent");
                        QString messageValueString = "";
                        if (decryptedDataJson["type"].toString() == "text") {
                            messageValueString = decryptedDataJson["value"]["text"].toString();
                        } else if (decryptedDataJson["type"].toString() == "file") {
                            messageValueString = "Файл: " + decryptedDataJson["value"]["name"].toString();
                        } else if (decryptedDataJson["type"].toString() == "sticker") {
                            QString stickerName = decryptedDataJson["value"]["name"].toString();
                            std::string stickerNameStd = stickerName.toStdString();

                            auto it = emojiNames.find(stickerNameStd);
                            if (it != emojiNames.end()) {
                                messageValueString = "Стикер: " + QString::fromStdString(it->second);
                            } else {
                                messageValueString = "Неизвестный стикер";
                            }
                        }
                        chatCard->setLatestMessage(messageValueString, decryptedDataJson["timestamp"].toString(), "sent");
                    }
                }

                break;
            }
        }
    }
}

void MainWindow::proccessOnChatConnectedEvent(const QString& chatId) {
    if (chatId != "initial-connection-response") {
        qDebug() << "Chat connected: " << chatId;
        for (chatCardState* chatCardStateObject_ : chatCardStateObjects) {
            if (chatCardStateObject_->chatId == chatId) {
                chatCardStateObject_->setOnlineStatus(true);

                if (openedChat != nullptr && openedChat->chatId == chatId) {
                    ui->chatDescription->setText("в сети");
                }

                QJsonObject messageSync;
                messageSync["type"] = "message-sync";
                QJsonObject messageSyncCredentials;
                messageSyncCredentials["chatId"] = chatId;
                messageSync["credentials"] = messageSyncCredentials;
                QJsonArray messageSyncData;

                // Sync messages
                if (!dbConnection.open()) {
                    qDebug() << "Error opening DB (proccessOnChatConnectedEvent):" << dbConnection.lastError().text();
                    return;
                }

                QString fetchMessagesQuery = QString("SELECT uuid FROM Messages WHERE chatId = '%1' AND state = 'init' ORDER BY datetime(timestamp) DESC ").arg(chatId);
                QSqlQuery query;

                query.prepare(fetchMessagesQuery);

                std::vector<QString> messageUuids;

                if (query.exec()) {
                    while (query.next()) {
                        QString uuid = query.value(0).toString();
                        messageUuids.push_back(uuid);
                    }
                    dbConnection.close();
                } else {
                    qDebug() << "Error executing query (proccessOnChatConnectedEvent):" << query.lastError().text();
                    dbConnection.close();
                }

                std::vector<messageState*> localMessageStateObjects;

                for (auto it = messageUuids.begin(); it != messageUuids.end(); ++it) {
                    messageState* messageStateObject = new class messageState(accountId, aesKeyFromPassword, chatId, *it, &dbConnection);
                    localMessageStateObjects.push_back(messageStateObject);
                }

                QByteArray iv = generateRandomIV();

                QJsonObject message;
                message["type"] = "message";
                QJsonObject credentials;
                credentials["chatId"] = chatCardStateObject_->chatId;
                QString ivString = iv.toBase64();
                credentials["iv"] = ivString;
                message["credentials"] = credentials;

                QJsonObject data;
                data["isBanned"] = chatCardStateObject_->isBanned;
                data["type"] = "ban-status";
                QJsonDocument dataJsonDoc(data);
                QByteArray encryptedData = encryptQStringAES(dataJsonDoc.toJson(QJsonDocument::Compact), chatCardStateObject_->chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
                QString dataString = encryptedData.toBase64();
                message["data"] = dataString;

                messageSyncData.append(message);

                for (messageState* messageStateObject_ : localMessageStateObjects) {
                    QJsonObject message_;
                    message_["type"] = "message";
                    QJsonObject credentials;
                    credentials["chatId"] = chatId;
                    QByteArray iv = generateRandomIV();
                    QString ivString = iv.toBase64();
                    credentials["iv"] = ivString;
                    message_["credentials"] = credentials;

                    QJsonObject data;
                    data["id"] = messageStateObject_->messageId;
                    data["type"] = messageStateObject_->type;
                    data["timestamp"] = messageStateObject_->timestampString;
                    data["sender"] = accountId;
                    data["state"] = "sent";
                    data["value"] = messageStateObject_->value;
                    QJsonDocument dataJsonDoc(data);
                    QByteArray encryptedData = encryptQStringAES(dataJsonDoc.toJson(QJsonDocument::Compact), chatCardStateObject_->chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
                    QString dataString = encryptedData.toBase64();
                    message_["data"] = dataString;

                    messageSyncData.append(message_);
                }

                if (messageSyncData.size() > 0) {
                    messageSync["data"] = messageSyncData;
                    peerConnection->sendMessage(messageSync, chatId);
                }
            }
        }
    }
}

void MainWindow::proccessOnChatDisconnectedEvent(const QString& chatId) {
    if (chatId != "initial-connection-response") {
        qDebug() << "Chat disconnected: " << chatId;
        for (chatCardState* chatCardStateObject_ : chatCardStateObjects) {
            if (chatCardStateObject_->chatId == chatId) {

                chatCardStateObject_->setOnlineStatus(false);

                if (openedChat != nullptr && openedChat->chatId == chatId) {
                    ui->chatDescription->setText("был(а) недавно");
                }
            }
        }
    }
}

void MainWindow::proccessInitialConnectionRequest(const QJsonDocument& connectionDetails) {
    quint16 connectionPort = connectionDetails["credentials"]["port"].toInteger();
    QString connectionIpAddress = connectionDetails["credentials"]["ipAddress"].toString();
    QString expirationString = connectionDetails["credentials"]["expire"].toString();
    QString connectionNickname = connectionDetails["credentials"]["nickname"].toString();
    QString connectionChatId = connectionDetails["credentials"]["chatId"].toString();
    QString connectionRsaPublicKey = connectionDetails["credentials"]["rsaPublicKey"].toString();
    addChatRequest *addChatRequestDialog = new class addChatRequest(this);
    connect(addChatRequestDialog, &addChatRequest::requestAccepted, this, &MainWindow::proccessRequestAccepted);
    connect(addChatRequestDialog, &addChatRequest::requestBanned, this, &MainWindow::proccessRequestBanned);
    connect(addChatRequestDialog, &addChatRequest::requestRejected, this, &MainWindow::proccessRequestRejected);
    addChatRequestDialog->setAttribute(Qt::WA_DeleteOnClose);
    addChatRequestDialog->setContent(expirationString, connectionNickname, connectionIpAddress, connectionPort, connectionChatId, connectionRsaPublicKey);
    addChatRequestDialog->exec();
}

void MainWindow::proccessRequestAccepted(const QString& connectionNickname, const QString& connectionIpAddress, const quint16& connectionPort, const QString& connectionChatId, const QString& rsaPublicKey) {
    QJsonObject message;
    message["type"] = "initial-connection-response";
    message["data"] = "accepted";
    QJsonObject credentials;
    credentials["chatId"] = connectionChatId;
    credentials["nickname"] = nickname;
    credentials["ipAddress"] = ipAddress;
    credentials["port"] = port;

    RSA *receivedRsaKey = rsaPublicKeyFromString(rsaPublicKey);
    AES_KEY *aesKey = generateAESKey();
    QString aesKeyString = aesKeyToString(aesKey);
    int encryptedAesKeyLength;
    unsigned char *encryptedAesKey = rsaEncryptAESKey(receivedRsaKey, (const unsigned char *)aesKey, sizeof(AES_KEY), &encryptedAesKeyLength);

    QByteArray encryptedAesKeyBase64 = QByteArray(reinterpret_cast<const char*>(encryptedAesKey), encryptedAesKeyLength).toBase64();
    QString encryptedAesKeyBase64String(encryptedAesKeyBase64);

    credentials["aesEncryptedKey"] = encryptedAesKeyBase64String;

    message["credentials"] = credentials;

    peerConnection->connectToPeer(connectionIpAddress, connectionPort, "initial-connection-response");
    connect(peerConnection, &PeerConnection::initialConnectionResponseConnected, [=]() {
        peerConnection->sendMessage(message, "initial-connection-response");
    });
    connect(peerConnection, &PeerConnection::initialConnectionResponseSent, [=]() {
        peerConnection->disconnectFromPeer("initial-connection-response");
        addChat(connectionChatId, connectionNickname, aesKeyString, connectionIpAddress, connectionPort, false, false, true);
    });
}

void MainWindow::proccessRequestBanned(const QString& connectionNickname, const QString& connectionIpAddress, const quint16& connectionPort, const QString& connectionChatId, const QString& rsaPublicKey) {
    QJsonObject message;
    message["type"] = "initial-connection-response";
    message["data"] = "banned";
    QJsonObject credentials;
    credentials["chatId"] = connectionChatId;
    credentials["nickname"] = ipAddress + ":" + portString;
    credentials["ipAddress"] = ipAddress;
    credentials["port"] = port;

    RSA *receivedRsaKey = rsaPublicKeyFromString(rsaPublicKey);
    AES_KEY *aesKey = generateAESKey();
    QString aesKeyString = aesKeyToString(aesKey);
    int encryptedAesKeyLength;
    unsigned char *encryptedAesKey = rsaEncryptAESKey(receivedRsaKey, (const unsigned char *)aesKey, sizeof(AES_KEY), &encryptedAesKeyLength);

    QByteArray encryptedAesKeyBase64 = QByteArray(reinterpret_cast<const char*>(encryptedAesKey), encryptedAesKeyLength).toBase64();
    QString encryptedAesKeyBase64String(encryptedAesKeyBase64);

    credentials["aesEncryptedKey"] = encryptedAesKeyBase64String;

    message["credentials"] = credentials;

    peerConnection->connectToPeer(connectionIpAddress, connectionPort, "initial-connection-response");
    connect(peerConnection, &PeerConnection::initialConnectionResponseConnected, [=]() {
        peerConnection->sendMessage(message, "initial-connection-response");
    });
    connect(peerConnection, &PeerConnection::initialConnectionResponseSent, [=]() {
        peerConnection->disconnectFromPeer("initial-connection-response");
        addChat(connectionChatId, connectionNickname, aesKeyString, connectionIpAddress, connectionPort, true, false, true);
    });
}

void MainWindow::proccessRequestRejected(const QString& connectionNickname, const QString& connectionIpAddress, const quint16& connectionPort, const QString& connectionChatId, const QString& rsaPublicKey) {
    peerConnection->connectToPeer(connectionIpAddress, connectionPort, connectionChatId);
    QJsonObject message;
    message["type"] = "initial-connection-response";
    message["data"] = "rejected";
    QJsonObject credentials;
    credentials["chatId"] = connectionChatId;
    message["credentials"] = credentials;

    peerConnection->connectToPeer(connectionIpAddress, connectionPort, "initial-connection-response");
    connect(peerConnection, &PeerConnection::initialConnectionResponseConnected, [=]() {
        peerConnection->sendMessage(message, "initial-connection-response");
    });
    connect(peerConnection, &PeerConnection::initialConnectionResponseSent, [=]() {
        peerConnection->disconnectFromPeer("initial-connection-response");
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showAccountChoiceDialog() {
    QSqlDatabase::removeDatabase("qt_sql_default_connection");

    dbConnection = QSqlDatabase::addDatabase("QSQLITE");
    dbConnection.setDatabaseName("mrChatDB.db");

    accountChoice *accountChoiceDialog = new accountChoice(this, &dbConnection);
    connect(accountChoiceDialog, &accountChoice::accountChosen, this, &MainWindow::switchAccount);
    accountChoiceDialog->setAttribute(Qt::WA_DeleteOnClose);
    accountChoiceDialog->exec();
}

void MainWindow::switchAccount(const QString& accountId) {
    if (accountId == "new") {
        addAccount *addAccountDialog = new addAccount(this, &dbConnection);
        connect(addAccountDialog, &addAccount::accountCreationCancelled, this, &MainWindow::showAccountChoiceDialog);
        connect(addAccountDialog, &addAccount::accountCreated, this, &MainWindow::loadUserAccount);
        addAccountDialog->setAttribute(Qt::WA_DeleteOnClose);
        addAccountDialog->exec();
    } else {
        logInAccount *logInAccountDialog = new logInAccount(this, &dbConnection);
        logInAccountDialog->loadUserAccount(accountId);
        connect(logInAccountDialog, &logInAccount::accountLogInCancelled, this, &MainWindow::showAccountChoiceDialog);
        connect(logInAccountDialog, &logInAccount::accountLoggedIn, this, &MainWindow::loadUserAccount);
        logInAccountDialog->setAttribute(Qt::WA_DeleteOnClose);
        logInAccountDialog->exec();
    }
}

void MainWindow::loadUserAccount(const QString& u_accountId, AES_KEY *u_aesKeyFromPassword) {

    show();

    ui->messageInput->installEventFilter(this);

    accountId = u_accountId;
    aesKeyFromPassword = u_aesKeyFromPassword;

    if (!dbConnection.open()) {
        qDebug() << "Error opening DB (selectUserByIdQuery):" << dbConnection.lastError().text();
        return;
    }

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString lastLoginDateTime = currentDateTime.toString(Qt::ISODate);

    QString updateAccountLoginDateQuery = "UPDATE Users SET lastLoginDate = :lastLoginDate WHERE uuid = :uuid;";
    QSqlQuery updateAccountQuery;
    updateAccountQuery.prepare(updateAccountLoginDateQuery);

    updateAccountQuery.bindValue(":lastLoginDate", lastLoginDateTime);
    updateAccountQuery.bindValue(":uuid", accountId);

    if (!updateAccountQuery.exec()) {
        qDebug() << "Error updating account icon (updateAccountIconQuery):" << updateAccountQuery.lastError().text();
        dbConnection.close();
        return;
    }

    QString selectUserByIdQuery = QString("SELECT * FROM Users WHERE uuid = '%1';").arg(accountId);

    QSqlQuery selectUserQuery(selectUserByIdQuery);
    if (!selectUserQuery.exec()) {
        qDebug() << "Error executing selectUserQuery:" << selectUserQuery.lastError().text();
        dbConnection.close();
        return;
    }

    if (selectUserQuery.next()) {
        nickname = selectUserQuery.value("username").toString();
        encryptedPassword = selectUserQuery.value("password").toString();
        port = selectUserQuery.value("port").toInt();
        portString = selectUserQuery.value("port").toString();
        iconName = selectUserQuery.value("iconName").toString();
    } else {
        dbConnection.close();
        return;
    }

    ipAddress = peerConnection->getCurrentIPAddress();

    closeOpenedChat();

    if (!peerConnection->start(port)) {
        qDebug() << "Failed to start peer!";
        setWindowTitle("MrChat. Не удалось подключиться. Перезагрузите приложение");
    } else {
        setWindowTitle("MrChat. Адрес для подключения: " + ipAddress + ":" + portString + " (" + nickname + ")");
    }

    connect(peerConnection, &PeerConnection::initialConnectionRequest, this, &MainWindow::proccessInitialConnectionRequest);
    connect(peerConnection, &PeerConnection::onChatConnectedEvent, this, &MainWindow::proccessOnChatConnectedEvent);
    connect(peerConnection, &PeerConnection::onChatDisconnectedEvent, this, &MainWindow::proccessOnChatDisconnectedEvent);
    connect(peerConnection, &PeerConnection::messageReceiving, this, &MainWindow::proccessMessageReceiving);
    connect(peerConnection, &PeerConnection::messageSyncReceiving, this, &MainWindow::proccessMessageSyncReceiving);
    connect(peerConnection, &PeerConnection::messageSyncSent, this, &MainWindow::proccessMessageSyncSent);
    connect(peerConnection, &PeerConnection::messageStateSent, this, &MainWindow::proccessMessageStateSent);
    connect(peerConnection, &PeerConnection::messageSent, this, &MainWindow::proccessMessageSent);

    // Render chats list

    QStringList tables = dbConnection.tables();

    if (tables.contains("Chats")) {

        QSqlQuery getChatsQuery;
        getChatsQuery.prepare("SELECT Chats.uuid "
                              "FROM Chats "
                              "WHERE Chats.accountId = :accountId");
        getChatsQuery.bindValue(":accountId", accountId);
        if (!getChatsQuery.exec()) {
            qDebug() << "Error executing getChatsQuery:" << getChatsQuery.lastError().text();
            dbConnection.close();
            return;
        }

        std::vector<QString> chatUuids;

        while (getChatsQuery.next()) {
            QString chatUuid = getChatsQuery.value("uuid").toString();
            chatUuids.push_back(chatUuid);
        }
        dbConnection.close();

        for (auto it = chatUuids.begin(); it != chatUuids.end(); ++it) {
            chatCardState* chatCardStateObject = new chatCardState(accountId, *it, aesKeyFromPassword, &dbConnection);
            chatCardStateObject->index = 1;
            chatCardStateObjects.push_back(chatCardStateObject);
        }

    }

    renderChatsList();

    connect(messagesListVerticalScrollBar, &QScrollBar::valueChanged, this, &MainWindow::scrollBarValueChanged);
}

void MainWindow::openChat(const QString& chatId) {

    qDebug() << "Opening chat: " << chatId;

    if (openedChat == nullptr || openedChat->chatId != chatId) {
        closeOpenedChat();
        enableTopBar();
        for (chatCardState* chatCardStateObject_ : chatCardStateObjects) {
            if (chatCardStateObject_->chatId == chatId) {

                openedChat = chatCardStateObject_;

                if (openedChat->isBanned || openedChat->isBannedByCompanion) {
                    disableBottomBar();
                } else {
                    enableBottomBar();
                }

                QTimer::singleShot(1, this, [=]() {
                    renderMessagesList("", true);
                });

                chatCardStateObject_->setActiveStatus(true);

                ui->chatTitle->setText(chatCardStateObject_->title);
                ui->chatDescription->setText(chatCardStateObject_->isOnline ? "в сети" : "был(а) недавно");

                break;
            }
        }
    } else if (openedChat != nullptr || openedChat->chatId == chatId) {
        QPropertyAnimation *animation = new QPropertyAnimation(messagesListVerticalScrollBar, "value");
        animation->setDuration(250);
        animation->setEasingCurve(QEasingCurve::InOutQuad);
        animation->setEndValue(messagesListVerticalScrollBar->maximum());
        animation->start();
    }
}

void MainWindow::disableTopBar() {
    ui->detailsButton->hide();
    ui->chatTitle->setText("MrChat");
    ui->chatDescription->setText("децентрализованный P2P-мессенджер");
}

void MainWindow::enableTopBar() {
    ui->detailsButton->show();
    ui->chatDescription->show();
}

void MainWindow::disableBottomBar() {
    ui->messageInput->setEnabled(false);
    ui->sendFileButton->setEnabled(false);
    ui->sendStickerButton->setEnabled(false);
    resetMessageInput();
}

void MainWindow::enableBottomBar() {
    ui->sendFileButton->setEnabled(true);
    ui->sendStickerButton->setEnabled(true);
    ui->messageInput->setEnabled(true);
}

void MainWindow::closeOpenedChat(const bool showHelpCenter) {

    disableTopBar();
    disableBottomBar();

    if (openedChat != nullptr) {
        openedChat->setActiveStatus(false);
        ui->chatTitle->setText("MrChat");
        ui->chatDescription->setText("Децентрализованный чат со сквозным шифрованием");
        openedChat = nullptr;
    }

    if (showHelpCenter) {

    for (messageState* messageStateObject_ : messageStateObjects) {
        if (messageStateObject_ != nullptr) {
            if (messageStateObject_->messageElement != nullptr) {
                delete messageStateObject_->messageElement;
                messageStateObject_->messageElement = nullptr;
            }
        }
    }

    messageStateObjects.clear();

    QLayoutItem *item;
    while ((item = ui->messageList->takeAt(0)) != nullptr) {
        if (item->widget() != nullptr) {
            delete item->widget();
        }
        delete item;
    }

    QFile file(":/faq/faq.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Error opening file:" << ":/faq/faq.json";
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(fileData);

    if (doc.isNull()) {
        qDebug() << "Error parsing JSON file";
    }

    if (!doc.isArray()) {
        qDebug() << "Root element is not a JSON array";
    }

    QJsonArray messages = doc.array();

    for (const QJsonValue& messageValue : messages) {
        if (!messageValue.isObject()) {
            qDebug() << "Invalid message format";
            continue;
        }

        QJsonObject message = messageValue.toObject();

        QString question = message["question"].toString();
        QString answer = message["answer"].toString();

        qDebug() << answer << question;

        messageState* messageQuestionStateObject__ = new class messageState("", nullptr, "", "question=​" + question.replace(" ", " "), nullptr);
        messageQuestionStateObject__->index = ui->messageList->count();
        renderMessage(messageQuestionStateObject__, "", false);

        messageState* messageAnswerStateObject__ = new class messageState("", nullptr, "", "answer=" + answer.replace(" ", " "), nullptr);
        messageAnswerStateObject__->index = ui->messageList->count();
        renderMessage(messageAnswerStateObject__, "", false);
    }

    messageState* messageStateObject__ = new class messageState("", nullptr, "", "info=Справка MrChat (v0.9.1)", nullptr);
    messageStateObject__->index = ui->messageList->count();
    renderMessage(messageStateObject__, "", false);

    } else {
        messageState* messageStateObject__ = new class messageState("", nullptr, "", "info=Выберите чат", nullptr);
        messageStateObject__->index = 0;
        renderMessage(messageStateObject__, "", false);
    }
}

void MainWindow::addChat(const QString& newChatUuid, const QString& newChatTitle, const QString& newChatAesKey, const QString& newChatIpAddress, const quint16& newChatPort, bool isBanned, bool isBannedByCompanion, bool isFromCompanion) {

    if (!dbConnection.open()) {
        qDebug() << "Error opening DB (addChat):" << dbConnection.lastError().text();
        return;
    }

    QString createChatTableQuery = "CREATE TABLE IF NOT EXISTS Chats ("
                                   "uuid TEXT PRIMARY KEY,"
                                   "accountId TEXT NOT NULL,"
                                   "icon BLOB NOT NULL,"
                                   "title BLOB NOT NULL,"
                                   "aesKey BLOB NOT NULL,"
                                   "iv BLOB NOT NULL,"
                                   "ipAddress BLOB NOT NULL,"
                                   "port BLOB NOT NULL,"
                                   "addressHash BLOB NOT NULL,"
                                   "isBanned BLOB NOT NULL,"
                                   "isBannedByCompanion BLOB NOT NULL,"
                                   "FOREIGN KEY(accountId) REFERENCES Users(uuid)"
                                   ");";

    QSqlQuery query;
    if (!query.exec(createChatTableQuery)) {
        qDebug() << "Ошибка при создании таблицы (createChatTableQuery):" << query.lastError().text();
        return;
    }

    QString insertChatQuery = "INSERT INTO Chats (uuid, accountId, icon, title, aesKey, iv, ipAddress, port, addressHash, isBanned, isBannedByCompanion) "
                              "VALUES (:uuid, :accountId, :icon, :title, :aesKey, :iv, :ipAddress, :port, :addressHash, :isBanned, :isBannedByCompanion);";

    query.prepare(insertChatQuery);
    query.bindValue(":uuid", newChatUuid);
    query.bindValue(":accountId", accountId);

    QByteArray iv = generateRandomIV();

    QByteArray e_newChatIcon = encryptQStringAES("dotted_line_face", aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    query.bindValue(":icon", QVariant(e_newChatIcon));
    QByteArray e_newChatTitle = encryptQStringAES(newChatTitle, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    query.bindValue(":title", QVariant(e_newChatTitle));
    QByteArray e_newChatAesKey = encryptQStringAES(newChatAesKey, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    query.bindValue(":aesKey", QVariant(e_newChatAesKey));
    query.bindValue(":iv", QVariant(iv));
    QByteArray e_newChatIpAddress = encryptQStringAES(newChatIpAddress, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    query.bindValue(":ipAddress", QVariant(e_newChatIpAddress));
    QString newChatPortString = QString::number(newChatPort);
    QByteArray e_newChatPort = encryptQStringAES(newChatPortString, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    query.bindValue(":port", QVariant(e_newChatPort));
    QString newChatAddressHash = QCryptographicHash::hash((newChatIpAddress + ":" + newChatPortString).toUtf8(), QCryptographicHash::Sha256).toHex();
    query.bindValue(":addressHash", newChatAddressHash);
    QString isBannedString = isBanned ? "true" : "false";
    QByteArray e_isBanned = encryptQStringAES(isBannedString, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    query.bindValue(":isBanned", QVariant(e_isBanned));
    QString isBannedByCompanionString = isBannedByCompanion ? "true" : "false";
    QByteArray e_isBannedByCompanion = encryptQStringAES(isBannedByCompanionString, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    query.bindValue(":isBannedByCompanion", QVariant(e_isBannedByCompanion));

    if (!query.exec()) {
        qDebug() << "Ошибка при добавлении чата (insertChatQuery):" << query.lastError().text();
        dbConnection.close();
        return;
    }

    dbConnection.close();

    qDebug() << "NEW CHAT ID" << newChatUuid;

    chatCardState* chatCardStateObject = new chatCardState(accountId, newChatUuid, aesKeyFromPassword, &dbConnection);
    chatCardStateObject->index = 1;

    qDebug() << chatCardStateObject->chatId << chatCardStateObject->title;

    if (isFromCompanion) {
        chatCardStateObject->isOnline = true;
    }

    chatCardStateObjects.push_back(chatCardStateObject);

    renderChatsList();
}

void MainWindow::renderChatsList(const QString& filterSubstring) {

    if (filterSubstring.isEmpty()) {
        for (chatCardState* chatCardStateObject_ : chatCardStateObjects) {
            if (chatCardStateObject_ != nullptr) {
                if (chatCardStateObject_->chatCardElement != nullptr) {
                    delete chatCardStateObject_->chatCardElement;
                    chatCardStateObject_->chatCardElement = nullptr;
                }
            }
        }

        QLayoutItem *item;
        while ((item = ui->chatCards->takeAt(0)) != nullptr) {
            if (item->widget() != nullptr) {
                delete item->widget();
            }
            delete item;
        }

        chatCardState* chatCardStateObject = new chatCardState(accountId, "new", aesKeyFromPassword, &dbConnection);
        chatCardStateObject->index = 0;
        renderChatCard(chatCardStateObject);

        quint16 chatCardIndex = 0;
        for (chatCardState* chatCardStateObject_ : chatCardStateObjects) {
            if (chatCardStateObject_ != nullptr) {
                chatCardStateObject_->index = ++chatCardIndex;
                renderChatCard(chatCardStateObject_);
            }
        }

        peerConnection->connectChats();
    } else {
        for (chatCardState* chatCardStateObject_ : chatCardStateObjects) {
            if (chatCardStateObject_ != nullptr && chatCardStateObject_->chatCardElement != nullptr && chatCardStateObject_->title.toLower().contains(filterSubstring.toLower())) {
                chatCardStateObject_->chatCardElement->show();
            } else if (chatCardStateObject_ != nullptr && chatCardStateObject_->chatCardElement != nullptr) {
                chatCardStateObject_->chatCardElement->hide();
            }
        }
    }

}

void MainWindow::renderChatCard(chatCardState* chatCardStateObject_) {
    if (chatCardStateObject_ != nullptr) {
        chatCardStateObject_->render();
        ui->chatCards->insertWidget(chatCardStateObject_->index, chatCardStateObject_->chatCardElement);
        connect(chatCardStateObject_->chatCardElement, &chatCard::chatCardClicked, this, [chatId = chatCardStateObject_->chatId, this]() {
            handleChatCardClick(chatId);
        });
    }
}

void MainWindow::handleChatCardClick(const QString& chatId) {
    if (chatId == "new") {
        class addChat *addChatDialog = new class addChat(this);
        connect(addChatDialog, &addChat::chatAdded, this, &MainWindow::handleChatAdded);
        addChatDialog->setAttribute(Qt::WA_DeleteOnClose);
        addChatDialog->exec();
    } else {
        openChat(chatId);
    }
}

void splitAddressAndPort(const QString &addressPortString, QString &ipAddress, quint16 &port)
{
    QStringList splitStrings = addressPortString.split(':');
    ipAddress = splitStrings.at(0);
    port = splitStrings.at(1).toInt();
}

void MainWindow::handleChatAdded(const QString& connectionAdress_) {
    if (!dbConnection.open()) {
        qDebug() << "Error opening DB (handleChatAdded):" << dbConnection.lastError().text();
        return;
    }

    QStringList tables = dbConnection.tables();

    if (tables.contains("ChatUsers")) {
        QString selectChatQuery = "SELECT Chats.uuid "
                                  "FROM Chats "
                                  "WHERE Chats.accountId = :accountId AND Chats.addressHash = :addressHash";

        QString addressHash = QCryptographicHash::hash(connectionAdress_.toUtf8(), QCryptographicHash::Sha256).toHex();

        QSqlQuery getChatWithSameCredentialsQuery;
        getChatWithSameCredentialsQuery.prepare(selectChatQuery);
        getChatWithSameCredentialsQuery.bindValue(":accountId", accountId);
        getChatWithSameCredentialsQuery.bindValue(":addressHash", addressHash);

        if (getChatWithSameCredentialsQuery.exec()) {
            if (getChatWithSameCredentialsQuery.next()) {
                QString uuid = getChatWithSameCredentialsQuery.value("uuid").toString();
                handleChatCardClick(uuid);
                dbConnection.close();
                return;
            } else {
                dbConnection.close();
            }
        } else {
            qDebug() << "Error executing selectChatQuery:" << getChatWithSameCredentialsQuery.lastError().text();
            dbConnection.close();
            return;
        }
    }

    if (peerConnection->stop()) {
        QString ipAddress_;
        quint16 port_;
        splitAddressAndPort(connectionAdress_, ipAddress_, port_);
        addchatprogress *addChatProgressDialog = new class addchatprogress(this);
        addChatProgressDialog->setContext(ipAddress_, port_, ipAddress, port, nickname);
        connect(addChatProgressDialog, &addchatprogress::chatAdded, this, &MainWindow::addChat);
        addChatProgressDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(addChatProgressDialog, &QDialog::finished, this, [this](){
            peerConnection->start(port);
        });
        addChatProgressDialog->exec();
    }
}

void MainWindow::addMessage(const QString& chatId, const QString& messageId, const QString& messageType, const QDateTime& messageTimestamp, const QString& messageTimestampString, const QString& messageSender, const QJsonObject& messageValue, const QString& messageStateString, const bool& shouldRenderMessage) {

    if (!dbConnection.open()) {
        qDebug() << "Error opening DB (addMessage):" << dbConnection.lastError().text();
        return;
    }

    QString createMessageTableQuery = "CREATE TABLE IF NOT EXISTS Messages ("
                                      "uuid TEXT PRIMARY KEY,"
                                      "chatId TEXT NOT NULL,"
                                      "iv BLOB NOT NULL,"
                                      "type BLOB NOT NULL,"
                                      "timestamp TEXT NOT NULL,"
                                      "sender TEXT NOT NULL,"
                                      "value BLOB NOT NULL,"
                                      "state TEXT NOT NULL,"
                                      "FOREIGN KEY(chatId) REFERENCES Chats(uuid)"
                                      ");";

    QSqlQuery query;
    if (!query.exec(createMessageTableQuery)) {
        qDebug() << "Error creating Messages table:" << query.lastError().text();
        dbConnection.close();
        return;
    }

    QString insertMessageQuery = "INSERT INTO Messages (uuid, chatId, iv, type, timestamp, sender, value, state) "
                                 "VALUES (:uuid, :chatId, :iv, :type, :timestamp, :sender, :value, :state);";

    QByteArray iv = generateRandomIV();

    query.prepare(insertMessageQuery);
    query.bindValue(":uuid", messageId);
    query.bindValue(":chatId", chatId);
    query.bindValue(":iv", QVariant(iv));
    QByteArray e_messageType = encryptQStringAES(messageType, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    query.bindValue(":type", QVariant(e_messageType));
    query.bindValue(":timestamp", messageTimestampString);
    query.bindValue(":sender", messageSender);
    QJsonDocument messageValueDoc(messageValue);
    QByteArray e_messageValue = encryptQStringAES(messageValueDoc.toJson(QJsonDocument::Compact), aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    query.bindValue(":value", e_messageValue);
    query.bindValue(":state", messageStateString);

    if (!query.exec()) {
        qDebug() << "Error inserting message:" << query.lastError().text();
        dbConnection.close();
        return;
    }

    dbConnection.close();

    if (shouldRenderMessage && openedChat != nullptr && chatId == openedChat->chatId) {
        if (int(messageStateObjects.size()) == 0 || messageTimestamp.date() != messageStateObjects.front()->timestamp.date()) {
            messageState* messageStateObject__ = new class messageState(accountId, aesKeyFromPassword, openedChat->chatId, "timestamp=" + messageTimestampString, &dbConnection);
            messageStateObject__->index = ui->messageList->count();
            renderMessage(messageStateObject__, "");
        }

        messageState* messageStateObject = new class messageState(accountId, aesKeyFromPassword, chatId, messageId, &dbConnection);
        messageStateObject->index = ui->messageList->count();
        messageStateObjects.insert(messageStateObjects.begin(), messageStateObject);
        renderMessage(messageStateObject);
    }

    for (chatCardState* chatCard : chatCardStateObjects) {
        if (chatCard != nullptr && chatCard->chatId == chatId) {
            QString messageValueString = "";
            if (messageType == "text") {
                messageValueString = messageValue["text"].toString();
            } else if (messageType == "file") {
                messageValueString = "Файл: " + messageValue["name"].toString();
            } else if (messageType == "sticker") {
                QString stickerName = messageValue["name"].toString();
                std::string stickerNameStd = stickerName.toStdString();

                auto it = emojiNames.find(stickerNameStd);
                if (it != emojiNames.end()) {
                    messageValueString = "Стикер: " + QString::fromStdString(it->second);
                } else {
                    messageValueString = "Неизвестный стикер";
                }
            }
            chatCard->setLatestMessage(messageValueString, messageTimestampString, messageStateString);
            if (accountId != messageSender) {
                chatCard->increaseUnreadAmout();
            }
            break;
        }
    }
}

QQueue<messageState*> messageRenderQueue;
bool renderingOfMessageInProgress = false;

void MainWindow::moveMessageRenderQueue(const QString& filterSubstring, const bool& enableAnimation) {
    if (!messageRenderQueue.isEmpty()) {
        renderingOfMessageInProgress = false;
        messageState* nextMessage = messageRenderQueue.dequeue();
        MainWindow::renderMessage(nextMessage, filterSubstring, enableAnimation);
    } else {
        renderingOfMessageInProgress = false;
    }
}

void MainWindow::renderMessage(messageState* messageStateObject_, const QString& filterSubstring, const bool& enableAnimation) {
    if (renderingOfMessageInProgress) {
        messageRenderQueue.enqueue(messageStateObject_);
        return;
    }

    renderingOfMessageInProgress = true;

    bool isAtBottom = (messagesListVerticalScrollBar->maximum() == messagesListVerticalScrollBar->value() || messageStateObject_->sender == accountId);

    if (messageStateObject_ != nullptr) {
        messageStateObject_->render(filterSubstring);
        if (messageStateObject_->type == "file") {
            connect(dynamic_cast<fileMessage*>(messageStateObject_->messageElement), &fileMessage::fileMessageClicked, this, [=](){
                QByteArray byteArray = QByteArray::fromBase64(messageStateObject_->value["data"].toString().toUtf8());
                QString filePath = QFileDialog::getSaveFileName(nullptr, "MrChat. Выбор расположения скачиваемого файла", QDir::homePath() + "/" + messageStateObject_->value["name"].toString());

                if (!filePath.isEmpty()) {
                    QFile file(filePath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(byteArray);
                        file.close();
                    }
                }
            });
        }

        ui->messageList->insertWidget(messageStateObject_->index, messageStateObject_->messageElement);

        if (enableAnimation) {
            if (messagesListVerticalScrollBar->maximum() == 0) {
                messageStateObject_->adjustSize();
            }

            shouldNotScrollMessagesDown = true;
            QTimer::singleShot(1, this, [=]() {
                int initialHeight = messageStateObject_->messageElement->height();
                if (messageStateObject_->type == "sticker") {
                    initialHeight = 240;
                }
                if (messagesListVerticalScrollBar->maximum() == 0) {
                    QPropertyAnimation *animation = new QPropertyAnimation(messageStateObject_->messageElement, "maximumHeight");
                    animation->setDuration(250);
                    animation->setEasingCurve(QEasingCurve::InOutQuad);
                    animation->setStartValue(0);
                    animation->setEndValue(initialHeight);
                    animation->start();
                    QTimer::singleShot(250, this, [=]() {
                        updateReadStatusForMessages();
                        shouldNotScrollMessagesDown = false;
                        QPropertyAnimation *animation = new QPropertyAnimation(messagesListVerticalScrollBar, "value");
                        animation->setDuration(250);
                        animation->setEasingCurve(QEasingCurve::InOutQuad);
                        animation->setEndValue(messagesListVerticalScrollBar->maximum());
                        animation->start();
                        moveMessageRenderQueue(filterSubstring, enableAnimation);
                    });
                } else {
                    if (isAtBottom) {
                        QTimer::singleShot(10, this, [=]() {
                            shouldNotScrollMessagesDown = false;
                            QPropertyAnimation *animation = new QPropertyAnimation(messagesListVerticalScrollBar, "value");
                            animation->setDuration(250);
                            animation->setEasingCurve(QEasingCurve::InOutQuad);
                            animation->setEndValue(messagesListVerticalScrollBar->maximum());
                            animation->start();
                            QTimer::singleShot(250, this, [=]() {
                                moveMessageRenderQueue(filterSubstring, enableAnimation);
                            });
                        });
                    } else {
                        moveMessageRenderQueue(filterSubstring, enableAnimation);
                    }
                }
            });
        } else {
            messagesListVerticalScrollBar->setValue(messageStateObject_->messageElement->height());
            moveMessageRenderQueue(filterSubstring, enableAnimation);
        }
    }
}

bool isRenderingMessagesList = false;
int previousScrollPosition = -1;

void MainWindow::scrollBarValueChanged(int value) {
    int threshold = 100;
    if (previousScrollPosition == -1) {
        previousScrollPosition = value;
    }
    if (previousScrollPosition - value >= 3 && value <= threshold && !isRenderingMessagesList) {
        renderMessagesList("", true, true);
    } else if (std::abs(previousScrollPosition - value) >= 20) {
        updateReadStatusForMessages();
        previousScrollPosition = value;
    }
}

void MainWindow::updateReadStatusForMessages() {
    if (openedChat == nullptr) {
        return;
    }
    for (messageState *messageStateObject_ : messageStateObjects) {
        if (messageStateObject_ != nullptr && messageStateObject_->messageElement != nullptr) {
            if (!messageStateObject_->messageElement->visibleRegion().isNull() && messageStateObject_->isFromCompanion && messageStateObject_->state == "sent") {
                AES_KEY *chatAesKey = openedChat->chatAesKey;
                QByteArray iv = generateRandomIV();

                QJsonObject message;
                message["type"] = "message-state";
                QJsonObject credentials;
                credentials["chatId"] = openedChat->chatId;
                QString ivString = iv.toBase64();
                credentials["iv"] = ivString;
                message["credentials"] = credentials;

                QJsonObject data;
                data["id"] = messageStateObject_->messageId;
                data["type"] = messageStateObject_->type;
                data["timestamp"] = messageStateObject_->timestampString;
                data["sender"] = messageStateObject_->sender;
                data["state"] = "read";
                data["value"] = messageStateObject_->value;
                QJsonDocument dataJsonDoc(data);
                QByteArray encryptedData = encryptQStringAES(dataJsonDoc.toJson(QJsonDocument::Compact), chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
                QString dataString = encryptedData.toBase64();
                message["data"] = dataString;

                peerConnection->sendMessage(message, openedChat->chatId);
            }
        }
    }
}

void MainWindow::proccessMessageStateSent(const QJsonDocument& messageDetails) {
    
    QString chatId = messageDetails["credentials"]["chatId"].toString();
    QByteArray iv = QByteArray::fromBase64(messageDetails["credentials"]["iv"].toString().toUtf8());
    QByteArray encryptedData = QByteArray::fromBase64(messageDetails["data"].toString().toUtf8());
    
    for (chatCardState* chatCard : chatCardStateObjects) {
        if (chatCard != nullptr && chatCard->chatId == chatId) {
            AES_KEY* chatAesKey = chatCard->chatAesKey;
            QString decryptedDataString = decryptQStringAES(encryptedData, chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
            QJsonDocument decryptedDataJson = QJsonDocument::fromJson(decryptedDataString.toUtf8());

            qDebug() << "PROCCESS MESSAGE STATE SENT" << decryptedDataJson["id"].toString();

            for (messageState* messageStateObject_ : messageStateObjects) {
                if (messageStateObject_ != nullptr && messageStateObject_->messageId == decryptedDataJson["id"].toString()) {
                    messageStateObject_->updateState("read");

                    QString messageValueString = "";
                    if (decryptedDataJson["type"].toString() == "text") {
                        messageValueString = decryptedDataJson["value"]["text"].toString();
                    } else if (decryptedDataJson["type"].toString() == "file") {
                        messageValueString = "Файл: " + decryptedDataJson["value"]["name"].toString();
                    } else if (decryptedDataJson["type"].toString() == "sticker") {
                        QString stickerName = decryptedDataJson["value"]["name"].toString();
                        std::string stickerNameStd = stickerName.toStdString();

                        auto it = emojiNames.find(stickerNameStd);
                        if (it != emojiNames.end()) {
                            messageValueString = "Стикер: " + QString::fromStdString(it->second);
                        } else {
                            messageValueString = "Неизвестный стикер";
                        }
                    }
                    chatCard->setLatestMessage(messageValueString, decryptedDataJson["timestamp"].toString(), "read");

                    if (accountId != decryptedDataJson["sender"].toString()) {
                        chatCard->decreaseUnreadAmout();
                    }
                }
            }

            break;
        }
    }
}

void MainWindow::proccessMessageSent(const QJsonDocument& messageDetails) {

    QString chatId = messageDetails["credentials"]["chatId"].toString();
    QByteArray iv = QByteArray::fromBase64(messageDetails["credentials"]["iv"].toString().toUtf8());
    QByteArray encryptedData = QByteArray::fromBase64(messageDetails["data"].toString().toUtf8());

    for (chatCardState* chatCard : chatCardStateObjects) {
        if (chatCard != nullptr && chatCard->chatId == chatId) {
            AES_KEY* chatAesKey = chatCard->chatAesKey;
            QString decryptedDataString = decryptQStringAES(encryptedData, chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
            QJsonDocument decryptedDataJson = QJsonDocument::fromJson(decryptedDataString.toUtf8());

            qDebug() << "PROCCESS MESSAGE SENT" << decryptedDataJson["id"].toString();

            for (messageState* messageStateObject_ : messageStateObjects) {
                if (messageStateObject_ != nullptr && messageStateObject_->messageId == decryptedDataJson["id"].toString()) {
                    messageStateObject_->updateState("sent");
                    QString messageValueString = "";
                    if (decryptedDataJson["type"].toString() == "text") {
                        messageValueString = decryptedDataJson["value"]["text"].toString();
                    } else if (decryptedDataJson["type"].toString() == "file") {
                        messageValueString = "Файл: " + decryptedDataJson["value"]["name"].toString();
                    } else if (decryptedDataJson["type"].toString() == "sticker") {
                        QString stickerName = decryptedDataJson["value"]["name"].toString();
                        std::string stickerNameStd = stickerName.toStdString();

                        auto it = emojiNames.find(stickerNameStd);
                        if (it != emojiNames.end()) {
                            messageValueString = "Стикер: " + QString::fromStdString(it->second);
                        } else {
                            messageValueString = "Неизвестный стикер";
                        }
                    }
                    chatCard->setLatestMessage(messageValueString, decryptedDataJson["timestamp"].toString(), "sent");
                }
            }

            break;
        }
    }
}

bool renderedAllMessagesFromDB = false;

void MainWindow::renderMessagesList(const QString& filterSubstring, const bool& reFetchMessages, const bool& movePaginationForwards) {

    if (reFetchMessages && !movePaginationForwards) {
        renderedAllMessagesFromDB = false;
        previousScrollPosition = -1;
    }

    if (movePaginationForwards && renderedAllMessagesFromDB) {
        return;
    }

    isRenderingMessagesList = true;
    int previousMaxScrollPosition = messagesListVerticalScrollBar->maximum();
    int previousScrollTopValue = messagesListVerticalScrollBar->value();

    if (!movePaginationForwards) {
        for (messageState* messageStateObject_ : messageStateObjects) {
            if (messageStateObject_ != nullptr) {
                if (messageStateObject_->messageElement != nullptr) {
                    delete messageStateObject_->messageElement;
                    messageStateObject_->messageElement = nullptr;
                }
            }
        }

        messageStateObjects.clear();

        QLayoutItem *item;
        while ((item = ui->messageList->takeAt(0)) != nullptr) {
            if (item->widget() != nullptr) {
                delete item->widget();
            }
            delete item;
        }
    }

    if (reFetchMessages && openedChat != nullptr) {
        if (!movePaginationForwards) {
            for (messageState* messageStateObject_ : messageStateObjects) {
                if (messageStateObject_ != nullptr) {
                    delete messageStateObject_;
                }
            }
        }

        if (!movePaginationForwards) {
            messageStateObjects.clear();
        }

        if (!dbConnection.open()) {
            qDebug() << "Error opening DB (renderMessagesList):" << dbConnection.lastError().text();
            return;
        }

        int queryLimit = 1;
        if (movePaginationForwards) {
            queryLimit = 25;
        }

        QString fetchMessagesQuery = "SELECT uuid FROM Messages WHERE chatId = ? ORDER BY datetime(timestamp) DESC LIMIT " + QString::number(queryLimit) + " OFFSET " + QString::number(messageStateObjects.size()) + ";";
        QSqlQuery query;

        query.prepare(fetchMessagesQuery);
        query.addBindValue(openedChat->chatId);

        std::vector<QString> messageUuids;

        if (query.exec()) {
            while (query.next()) {
                QString uuid = query.value(0).toString();
                messageUuids.push_back(uuid);
            }
            dbConnection.close();
        } else {
            qDebug() << "Error executing query (renderMessagesList):" << query.lastError().text();
            dbConnection.close();
        }

        std::vector<messageState*> localMessageStateObjects;

        for (auto it = messageUuids.begin(); it != messageUuids.end(); ++it) {
            messageState* messageStateObject = new class messageState(accountId, aesKeyFromPassword, openedChat->chatId, *it, &dbConnection);
            messageStateObject->index = 0;
            localMessageStateObjects.push_back(messageStateObject);
        }

        int oldMessageStateObjectsSize = messageStateObjects.size();

        messageStateObjects.insert(messageStateObjects.end(), localMessageStateObjects.begin(), localMessageStateObjects.end());

        if (reFetchMessages && !movePaginationForwards) {
            if (openedChat->isBanned) {
                messageState* messageStateObject__ = new class messageState(accountId, aesKeyFromPassword, openedChat->chatId, "info=Вы заблокировали собеседника", &dbConnection);
                messageStateObject__->index = 0;
                renderMessage(messageStateObject__, filterSubstring, false);
            } else if (openedChat->isBannedByCompanion) {
                messageState* messageStateObject__ = new class messageState(accountId, aesKeyFromPassword, openedChat->chatId, "info=Собеседник заблокировал Вас", &dbConnection);
                messageStateObject__->index = 0;
                renderMessage(messageStateObject__, filterSubstring, false);
            }
        }

        int messageStateObjectIndex = -1;

        for (messageState* messageStateObject_ : localMessageStateObjects) {
            renderMessage(messageStateObject_, filterSubstring, false);
            ++messageStateObjectIndex;

            if (messageStateObjectIndex <= int(localMessageStateObjects.size()) - 2 && messageStateObject_->timestamp.date() != messageStateObjects.at(oldMessageStateObjectsSize + 1 + messageStateObjectIndex)->timestamp.date()) {
                messageState* messageStateObject__ = new class messageState(accountId, aesKeyFromPassword, openedChat->chatId, "timestamp=" + messageStateObject_->timestampString, &dbConnection);
                messageStateObject__->index = 0;
                renderMessage(messageStateObject__, filterSubstring, false);
            }
        }

        localMessageStateObjects.clear();

        if (int(messageUuids.size()) < queryLimit) {
            renderedAllMessagesFromDB = true;

            if (int(messageStateObjects.size()) > 0) {
                messageState* messageStateObject___ = new class messageState(accountId, aesKeyFromPassword, openedChat->chatId, "timestamp=" + messageStateObjects.back()->timestampString, &dbConnection);
                messageStateObject___->index = 0;
                renderMessage(messageStateObject___, filterSubstring, false);
            }

            messageState* messageStateObject__ = new class messageState(accountId, aesKeyFromPassword, openedChat->chatId, "info=Чат создан", &dbConnection);
            messageStateObject__->index = 0;
            renderMessage(messageStateObject__, filterSubstring, false);
        }

        QTimer::singleShot(10, this, [=]() {
            if (!movePaginationForwards && messageUuids.size() > 0) {
                renderMessagesList(filterSubstring, true, true);
            } else {
                messagesListVerticalScrollBar->setValue(messagesListVerticalScrollBar->maximum() - previousMaxScrollPosition + previousScrollTopValue);
            }
            updateReadStatusForMessages();
            isRenderingMessagesList = false;
            previousScrollPosition = -1;
        });
    }
}

void MainWindow::on_sendMessageButton_clicked()
{
    sendTextMessage();
}

void MainWindow::sendTextMessage() {

    QString messageValueText = ui->messageInput->toPlainText().trimmed();

    if (openedChat != nullptr && !messageValueText.isEmpty()) {

        QDateTime messageTimestamp = QDateTime::currentDateTime();
        messageTimestamp.setTimeZone(QTimeZone("Europe/Moscow"));
        QString messageTimestampString = messageTimestamp.toString(Qt::ISODate);
        QString messageType = "text";
        QString messageId = QUuid::createUuid().toString().mid(1, 36);
        QString messageState = "init";
        QJsonObject messageValue;
        messageValue["text"] = messageValueText;

        addMessage(openedChat->chatId, messageId, messageType, messageTimestamp, messageTimestampString, accountId, messageValue, messageState);

        AES_KEY *chatAesKey = openedChat->chatAesKey;
        QByteArray iv = generateRandomIV();

        QJsonObject message;
        message["type"] = "message";
        QJsonObject credentials;
        credentials["chatId"] = openedChat->chatId;
        QString ivString = iv.toBase64();
        credentials["iv"] = ivString;
        message["credentials"] = credentials;

        QJsonObject data;
        data["id"] = messageId;
        data["type"] = messageType;
        data["timestamp"] = messageTimestampString;
        data["sender"] = accountId;
        data["state"] = "sent";
        data["value"] = messageValue;
        QJsonDocument dataJsonDoc(data);
        QByteArray encryptedData = encryptQStringAES(dataJsonDoc.toJson(QJsonDocument::Compact), chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
        QString dataString = encryptedData.toBase64();
        message["data"] = dataString;

        peerConnection->sendMessage(message, openedChat->chatId);

        resetMessageInput();

    }
}

void MainWindow::resetMessageInput() {
    ui->messageInput->setText("");
    ui->messageInput->setFocus();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->messageInput && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            if (keyEvent->modifiers() == Qt::ShiftModifier || keyEvent->modifiers() == Qt::AltModifier || keyEvent->modifiers() == Qt::ControlModifier) {
                QKeyEvent *customEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
                QCoreApplication::postEvent(obj, customEvent);
                return true;
            } else {
                sendTextMessage();
                return true;
            }
        }
    }
    return false;
}

void MainWindow::on_messageInput_textChanged()
{
    if (ui->messageInput->toPlainText().length() > 0) {
        ui->sendMessageButton->setEnabled(true);
    } else {
        ui->sendMessageButton->setEnabled(false);
    }

    int contentHeight = ui->messageInput->document()->size().height();
    if (contentHeight > 150) {
        contentHeight = 150;
    }

    if (ui->messageInput->height() != contentHeight) {
        if (messagesListVerticalScrollBar->maximum() == messagesListVerticalScrollBar->value()) {
            QTimer::singleShot(2, this, [=]() {
                if (shouldNotScrollMessagesDown) {
                    return;
                }
                messagesListVerticalScrollBar->setValue(messagesListVerticalScrollBar->maximum());
            });
        }

        ui->messageInput->setMinimumHeight(contentHeight);
        ui->messageInput->setMaximumHeight(contentHeight);
    }

}

void MainWindow::on_sendFileButton_clicked()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("MrChat. Выбор файлов для отправки"), QDir::homePath());

    for (const QString& fileName : fileNames) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {

            QDateTime messageTimestamp = QDateTime::currentDateTime();
            messageTimestamp.setTimeZone(QTimeZone("Europe/Moscow"));
            QString messageTimestampString = messageTimestamp.toString(Qt::ISODate);
            QString messageType = "file";
            QString messageId = QUuid::createUuid().toString().mid(1, 36);
            QString messageState = "init";
            QJsonObject messageValue;
            messageValue["name"] = QFileInfo(fileName).fileName();
            messageValue["size"] = file.size();
            messageValue["data"] = QString::fromLatin1(file.readAll().toBase64());
            file.close();

            addMessage(openedChat->chatId, messageId, messageType, messageTimestamp, messageTimestampString, accountId, messageValue, messageState);

            AES_KEY *chatAesKey = openedChat->chatAesKey;
            QByteArray iv = generateRandomIV();

            QJsonObject message;
            message["type"] = "message";
            QJsonObject credentials;
            credentials["chatId"] = openedChat->chatId;
            QString ivString = iv.toBase64();
            credentials["iv"] = ivString;
            message["credentials"] = credentials;

            QJsonObject data;
            data["id"] = messageId;
            data["type"] = messageType;
            data["timestamp"] = messageTimestampString;
            data["sender"] = accountId;
            data["state"] = "sent";
            data["value"] = messageValue;
            QJsonDocument dataJsonDoc(data);
            QByteArray encryptedData = encryptQStringAES(dataJsonDoc.toJson(QJsonDocument::Compact), chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
            QString dataString = encryptedData.toBase64();
            message["data"] = dataString;

            peerConnection->sendMessage(message, openedChat->chatId);

            encryptedData.clear();

            qDebug() << "File sent:" << fileName;
        } else {
            qDebug() << "Failed to open file:" << fileName;
        }
    }
}


void MainWindow::on_sendStickerButton_clicked()
{
    emojisPicker *emojisPickerDialog = new emojisPicker(this, "MrChat. Выбор стикера");
    emojisPickerDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(emojisPickerDialog, &emojisPicker::emojiPicked, this, &MainWindow::sendStickerMessage);
    emojisPickerDialog->exec();
}

void MainWindow::sendStickerMessage(const QString& stickerName) {
    qDebug() << "SENDING STICKER" << stickerName;

    if (openedChat != nullptr && !stickerName.isEmpty()) {

        QDateTime messageTimestamp = QDateTime::currentDateTime();
        messageTimestamp.setTimeZone(QTimeZone("Europe/Moscow"));
        QString messageTimestampString = messageTimestamp.toString(Qt::ISODate);
        QString messageType = "sticker";
        QString messageId = QUuid::createUuid().toString().mid(1, 36);
        QString messageState = "init";
        QJsonObject messageValue;
        messageValue["name"] = stickerName;

        addMessage(openedChat->chatId, messageId, messageType, messageTimestamp, messageTimestampString, accountId, messageValue, messageState);

        AES_KEY *chatAesKey = openedChat->chatAesKey;
        QByteArray iv = generateRandomIV();

        QJsonObject message;
        message["type"] = "message";
        QJsonObject credentials;
        credentials["chatId"] = openedChat->chatId;
        QString ivString = iv.toBase64();
        credentials["iv"] = ivString;
        message["credentials"] = credentials;

        QJsonObject data;
        data["id"] = messageId;
        data["type"] = messageType;
        data["timestamp"] = messageTimestampString;
        data["sender"] = accountId;
        data["state"] = "sent";
        data["value"] = messageValue;
        QJsonDocument dataJsonDoc(data);
        QByteArray encryptedData = encryptQStringAES(dataJsonDoc.toJson(QJsonDocument::Compact), chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
        QString dataString = encryptedData.toBase64();
        message["data"] = dataString;

        peerConnection->sendMessage(message, openedChat->chatId);

        resetMessageInput();

    }
}

void MainWindow::on_sideBarSearch_textChanged(const QString &arg1)
{
    renderChatsList(ui->sideBarSearch->text());
}


void MainWindow::on_detailsButton_clicked()
{
    chatSettings *chatSettingsDialog = new chatSettings(this, openedChat->ipAddress + ":" + openedChat->portString, openedChat->title, openedChat->iconName, openedChat->isBanned);
    chatSettingsDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(chatSettingsDialog, &chatSettings::chatIconSelected, this, &MainWindow::updateChatIcon);
    connect(chatSettingsDialog, &chatSettings::chatDataUpdated, this, &MainWindow::updateChatData);
    connect(chatSettingsDialog, &chatSettings::chatBanned, this, &MainWindow::updateChatBan);
    connect(chatSettingsDialog, &chatSettings::chatRemoved, this, &MainWindow::removeChat);
    chatSettingsDialog->exec();
}

void MainWindow::updateChatIcon(const QString& iconName) {
    openedChat->setNewIcon(iconName);
}

void MainWindow::updateChatData(const QString& newConnectionAddress, const QString& newNickname) {
    QString ipAddress_;
    quint16 port_;
    splitAddressAndPort(newConnectionAddress, ipAddress_, port_);
    openedChat->setNewData(ipAddress_, port_, newNickname);
    ui->chatTitle->setText(openedChat->title);
}

void MainWindow::updateChatBan() {
    bool isBannedUpdated = !openedChat->isBanned;
    openedChat->setChatBanned(isBannedUpdated, openedChat->isBannedByCompanion);
    QString prevOpenedChat = openedChat->chatId;
    closeOpenedChat();
    openChat(prevOpenedChat);

    AES_KEY *chatAesKey = openedChat->chatAesKey;
    QByteArray iv = generateRandomIV();

    QJsonObject message;
    message["type"] = "message";
    QJsonObject credentials;
    credentials["chatId"] = openedChat->chatId;
    QString ivString = iv.toBase64();
    credentials["iv"] = ivString;
    message["credentials"] = credentials;

    QJsonObject data;
    data["isBanned"] = isBannedUpdated;
    data["type"] = "ban-status";
    QJsonDocument dataJsonDoc(data);
    QByteArray encryptedData = encryptQStringAES(dataJsonDoc.toJson(QJsonDocument::Compact), chatAesKey, reinterpret_cast<unsigned char*>(iv.data()));
    QString dataString = encryptedData.toBase64();
    message["data"] = dataString;

    peerConnection->sendMessage(message, openedChat->chatId);
}

void MainWindow::removeChat() {

    QString prevOpenedChat = openedChat->chatId;
    closeOpenedChat();

    if (!dbConnection.open()) {
        qDebug() << "Error opening DB (deleteChatAndMessages):" << dbConnection.lastError().text();
        return;
    }

    QString deleteMessagesQuery = "DELETE FROM Messages WHERE chatId = :chatId";
    QSqlQuery query;
    query.prepare(deleteMessagesQuery);
    query.bindValue(":chatId", prevOpenedChat);
    if (!query.exec()) {
        qDebug() << "Error deleting messages (deleteChatAndMessages):" << query.lastError().text();
        dbConnection.close();
        return;
    }

    QString deleteChatQuery = "DELETE FROM Chats WHERE uuid = :chatId";
    query.prepare(deleteChatQuery);
    query.bindValue(":chatId", prevOpenedChat);
    if (!query.exec()) {
        qDebug() << "Error deleting chat (deleteChatAndMessages):" << query.lastError().text();
        dbConnection.close();
        return;
    }

    dbConnection.close();

    for (auto it = chatCardStateObjects.begin(); it != chatCardStateObjects.end(); ++it) {
        chatCardState* chatCard = *it;
        if (chatCard != nullptr && chatCard->chatId == prevOpenedChat) {
            delete chatCard;
            chatCardStateObjects.erase(it);
            break;
        }
    }

    renderChatsList();
    renderMessagesList();
}

void MainWindow::on_settingsButton_clicked()
{
    accountSettings *accountSettingsDialog = new accountSettings(this, this->port, this->nickname, this->iconName);
    accountSettingsDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(accountSettingsDialog, &accountSettings::accountIconSelected, this, &MainWindow::updateAccountIcon);
    connect(accountSettingsDialog, &accountSettings::accountDataUpdated, this, &MainWindow::updateAccountData);
    connect(accountSettingsDialog, &accountSettings::accountLogout, this, &MainWindow::logoutAccount);
    connect(accountSettingsDialog, &accountSettings::accountRemoved, this, &MainWindow::removeAccount);
    accountSettingsDialog->exec();
}

void MainWindow::updateAccountIcon(const QString& newIconName) {
    if (!dbConnection.open()) {
        qDebug() << "Error opening DB (updateAccountIcon):" << dbConnection.lastError().text();
        return;
    }

    QString updateAccountIconQuery = "UPDATE Users SET iconName = :icon WHERE uuid = :uuid;";
    QSqlQuery updateAccountQuery;
    updateAccountQuery.prepare(updateAccountIconQuery);

    updateAccountQuery.bindValue(":icon", newIconName);
    updateAccountQuery.bindValue(":uuid", accountId);

    if (!updateAccountQuery.exec()) {
        qDebug() << "Error updating account icon (updateAccountIconQuery):" << updateAccountQuery.lastError().text();
        dbConnection.close();
        return;
    }

    dbConnection.close();

    iconName = newIconName;
}

void MainWindow::updateAccountData(const qint16& newPort, const QString& newNickname) {
    if (!dbConnection.open()) {
        qDebug() << "Error opening DB (updateAccountIcon):" << dbConnection.lastError().text();
        return;
    }

    QString updateAccountIconQuery = "UPDATE Users SET username = :username, port = :port WHERE uuid = :uuid;";
    QSqlQuery updateAccountQuery;
    updateAccountQuery.prepare(updateAccountIconQuery);

    updateAccountQuery.bindValue(":username", newNickname);
    updateAccountQuery.bindValue(":port", newPort);
    updateAccountQuery.bindValue(":uuid", accountId);

    if (!updateAccountQuery.exec()) {
        qDebug() << "Error updating account icon (updateAccountIconQuery):" << updateAccountQuery.lastError().text();
        dbConnection.close();
        return;
    }

    dbConnection.close();

    port = newPort;
    portString = QString::number(port);
    nickname = newNickname;
    setWindowTitle("MrChat. Адрес для подключения: " + ipAddress + ":" + portString + " (" + nickname + ")");
}

void MainWindow::logoutAccount() {
    close();
}

void MainWindow::removeAccount() {
    if (!dbConnection.open()) {
        qDebug() << "Error opening DB (updateAccountIcon):" << dbConnection.lastError().text();
        return;
    }

    QString deleteMessagesQuery = "DELETE FROM Messages WHERE chatId IN ( SELECT uuid FROM Chats WHERE accountId = :accountId );";
    QSqlQuery deleteMessages(dbConnection);

    deleteMessages.prepare(deleteMessagesQuery);
    deleteMessages.bindValue(":accountId", accountId);

    if (!deleteMessages.exec()) {
        qDebug() << "Error deleting messages:" << deleteMessages.lastError().text();
        dbConnection.close();
        return;
    }

    QString deleteChatsQuery = "DELETE FROM Chats WHERE accountId = :accountId;";
    QSqlQuery deleteChats(dbConnection);

    deleteChats.prepare(deleteChatsQuery);
    deleteChats.bindValue(":accountId", accountId);

    if (!deleteChats.exec()) {
        qDebug() << "Error deleting chats:" << deleteMessages.lastError().text();
        dbConnection.close();
        return;
    }

    QString deleteUserQuery = "DELETE FROM Users WHERE uuid = :accountId;";
    QSqlQuery deleteUser(dbConnection);

    deleteUser.prepare(deleteUserQuery);
    deleteUser.bindValue(":accountId", accountId);

    if (!deleteUser.exec()) {
        qDebug() << "Error deleting user:" << deleteUser.lastError().text();
        dbConnection.close();
        return;
    }

    dbConnection.close();

    close();
}

void MainWindow::on_faqButton_clicked()
{
    closeOpenedChat(true);
}

