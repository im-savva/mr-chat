#include "chatcardstate.h"
#include "chatcard.h"
#include "encryption.h"
#include "messagestate.h"
#include "qcryptographichash.h"
#include "qsqlerror.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>

chatCardState::chatCardState(const QString& accountId_, QString chatId_, AES_KEY *aesKeyFromPassword_, QSqlDatabase *dbConnection_) {
    accountId = accountId_;
    chatId = chatId_;
    aesKeyFromPassword = aesKeyFromPassword_;
    dbConnection = dbConnection_;
    loadData();
}

chatCard* chatCardState::render() {
    if (chatId.isEmpty()) {
        qDebug() << "Chat ID is empty. Chat card could not be rendered.";
        return nullptr;
    }
    if (chatCardElement != nullptr) {
        qDebug() << "ALREADY RENDERED";
        return nullptr;
    }

    chatCardElement = new chatCard();

    chatCardElement->setId(chatId);
    chatCardElement->setTitle(title);
    chatCardElement->setUnreadAmount(unreadAmount == 0 ? "" : unreadAmount > 9 ? "⦿" : QString::number(unreadAmount));
    chatCardElement->setIsOnline(isOnline);

    chatCardElement->setObjectName("chatCard");
    setActiveStatus(isActive);

    chatCardElement->setIcon(iconName);
    return chatCardElement;
}

void chatCardState::setLatestMessage(const QString& value, const QString& timestampString, const QString& state) {
    if (latestMessageTimestampString.isEmpty() || (!timestampString.isEmpty() && QDateTime::fromString(latestMessageTimestampString, Qt::ISODate) < QDateTime::fromString(timestampString, Qt::ISODate)) || (latestMessageState != state && QDateTime::fromString(latestMessageTimestampString, Qt::ISODate) == QDateTime::fromString(timestampString, Qt::ISODate))) {
        latestMessageValue = value;
        latestMessageTimestampString = timestampString;
        latestMessageState = state;
    }
    updateLatestMessage();
}

void chatCardState::updateLatestMessage() {
    if (chatCardElement != nullptr) {
        if (isBanned) {
            chatCardElement->setLatestMessage("Вы заблокировали собеседника", "", "");
        } else if (isBannedByCompanion) {
            chatCardElement->setLatestMessage("Собеседник заблокировал Вас", "", "");
        } else {
            chatCardElement->setLatestMessage(latestMessageValue, latestMessageTimestampString, latestMessageState + (isActive ? "-white" : ""));
        }
    }
}

void chatCardState::setOnlineStatus(const bool& onlineStatus) {
    isOnline = onlineStatus;
    if (chatCardElement != nullptr) {
        chatCardElement->setIsOnline(isOnline);
    }
}

void chatCardState::setNewIcon(const QString& newIconName) {
    if (!dbConnection->open()) {
        qDebug() << "Error opening DB (updateChatIcon):" << dbConnection->lastError().text();
        return;
    }

    QString selectChatIconQuery = "SELECT iv FROM Chats WHERE uuid = :uuid;";
    QSqlQuery query;
    query.prepare(selectChatIconQuery);
    query.bindValue(":uuid", chatId);

    if (!query.exec() || !query.next()) {
        qDebug() << "Error retrieving chat icon (selectChatIconQuery):" << query.lastError().text();
        dbConnection->close();
        return;
    }
    QByteArray iv = query.value("iv").toByteArray();

    QString updateChatIconQuery = "UPDATE Chats SET icon = :icon WHERE uuid = :uuid;";
    QSqlQuery updateChatQuery;
    updateChatQuery.prepare(updateChatIconQuery);

    QByteArray e_newIcon = encryptQStringAES(newIconName, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    updateChatQuery.bindValue(":icon", QVariant(e_newIcon));
    updateChatQuery.bindValue(":uuid", chatId);

    if (!updateChatQuery.exec()) {
        qDebug() << "Error updating chat icon (updateChatIconQuery):" << updateChatQuery.lastError().text();
        dbConnection->close();
        return;
    }

    dbConnection->close();

    iconName = newIconName;
    chatCardElement->setIcon(iconName);
}

void chatCardState::setNewData(const QString& newIpAddress, const quint16& newPort, const QString& newNickname) {
    if (!dbConnection->open()) {
        qDebug() << "Error opening DB (updateChatIcon):" << dbConnection->lastError().text();
        return;
    }

    QString selectChatDataQuery = "SELECT iv FROM Chats WHERE uuid = :uuid;";
    QSqlQuery query;
    query.prepare(selectChatDataQuery);
    query.bindValue(":uuid", chatId);

    if (!query.exec() || !query.next()) {
        qDebug() << "Error retrieving chat icon (selectChatDataQuery):" << query.lastError().text();
        dbConnection->close();
        return;
    }
    QByteArray iv = query.value("iv").toByteArray();

    QString updateChatDataQuery = "UPDATE Chats SET ipAddress = :ipAddress, port = :port, addressHash = :addressHash, title = :title WHERE uuid = :uuid;";
    QSqlQuery updateChatQuery;
    updateChatQuery.prepare(updateChatDataQuery);

    QByteArray e_newChatTitle = encryptQStringAES(newNickname, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    updateChatQuery.bindValue(":title", QVariant(e_newChatTitle));
    QByteArray e_newChatIpAddress = encryptQStringAES(newIpAddress, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    updateChatQuery.bindValue(":ipAddress", QVariant(e_newChatIpAddress));
    QString newPortString = QString::number(newPort);
    QByteArray e_newChatPort = encryptQStringAES(newPortString, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    updateChatQuery.bindValue(":port", QVariant(e_newChatPort));
    QString newChatAddressHash = QCryptographicHash::hash((newIpAddress + ":" + newPortString).toUtf8(), QCryptographicHash::Sha256).toHex();
    updateChatQuery.bindValue(":addressHash", newChatAddressHash);

    updateChatQuery.bindValue(":uuid", chatId);

    if (!updateChatQuery.exec()) {
        qDebug() << "Error updating chat data (updateChatDataQuery):" << updateChatQuery.lastError().text();
        dbConnection->close();
        return;
    }

    dbConnection->close();

    title = newNickname;
    port = newPort;
    portString = newPortString;
    ipAddress = newIpAddress;

    chatCardElement->setTitle(title);
}

void chatCardState::setChatBanned(const bool& isBanned_, const bool& isBannedByCompanion_) {
    if (!dbConnection->open()) {
        qDebug() << "Error opening DB (updateChatIcon):" << dbConnection->lastError().text();
        return;
    }

    QString selectChatIconQuery = "SELECT iv FROM Chats WHERE uuid = :uuid;";
    QSqlQuery query;
    query.prepare(selectChatIconQuery);
    query.bindValue(":uuid", chatId);

    if (!query.exec() || !query.next()) {
        qDebug() << "Error retrieving chat icon (selectChatIconQuery):" << query.lastError().text();
        dbConnection->close();
        return;
    }
    QByteArray iv = query.value("iv").toByteArray();

    QString updateChatBannedQuery = "UPDATE Chats SET isBanned = :isBanned, isBannedByCompanion = :isBannedByCompanion WHERE uuid = :uuid;";
    QSqlQuery updateChatQuery;
    updateChatQuery.prepare(updateChatBannedQuery);

    QString isBannedString = isBanned_ ? "true" : "false";
    QByteArray e_isBannedString = encryptQStringAES(isBannedString, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    updateChatQuery.bindValue(":isBanned", QVariant(e_isBannedString));

    QString isBannedByCompanionString = isBannedByCompanion_ ? "true" : "false";
    QByteArray e_isBannedByCompanionString = encryptQStringAES(isBannedByCompanionString, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
    updateChatQuery.bindValue(":isBannedByCompanion", QVariant(e_isBannedByCompanionString));

    updateChatQuery.bindValue(":uuid", chatId);

    if (!updateChatQuery.exec()) {
        qDebug() << "Error updating chat icon (updateChatIconQuery):" << updateChatQuery.lastError().text();
        dbConnection->close();
        return;
    }

    dbConnection->close();

    isBanned = isBanned_;
    isBannedByCompanion = isBannedByCompanion_;

    updateLatestMessage();
}

void chatCardState::setActiveStatus(const bool& activeStatus) {
    isActive = activeStatus;
    if (chatCardElement != nullptr) {
        if (isActive) {
            chatCardElement->setStyleSheet(R"(
#MainWindow #chatCard #chatCardContainer {
    background: rgb(0,122,255);
}

#MainWindow #chatCard #chatCardContainer:hover {
    background: rgba(0,122,255,0.85);
}

#MainWindow #chatCard #latestMessageDate,
#MainWindow #chatCard #latestMessage {
    color: rgb(255,255,255);
}

#MainWindow #chatCard #chatTitle {
    color: rgb(255,255,255);
}

#MainWindow #chatCard #unreadAmount {
    border: none;
    background: rgb(255,255,255);
    color: rgb(0,122,255);
    border-radius: 8px;
    padding: 2px;
}
)");
        } else {
            chatCardElement->setStyleSheet(R"(
#MainWindow #chatCard #chatCardContainer {
    background: rgb(255,255,255);
}

#MainWindow #chatCard #chatCardContainer:hover {
    background: rgb(229,229,234);
}

#MainWindow #chatCard #latestMessageDate,
#MainWindow #chatCard #latestMessage {
    color: rgba(0,0,0,0.5);
}

#MainWindow #chatCard #unreadAmount {
    border: none;
    background: rgb(0,122,255);
    border-radius: 8px;
    color: rgb(255,255,255);
    padding: 2px;
}
)");
        }
        updateLatestMessage();
    }
}

void chatCardState::increaseUnreadAmout() {
    unreadAmount++;
    qDebug() << "INCREASING UNREAD AMOUNT:" << unreadAmount;
    chatCardElement->setUnreadAmount(unreadAmount == 0 ? "" : unreadAmount > 9 ? "⦿" : QString::number(unreadAmount));
}

void chatCardState::decreaseUnreadAmout() {
    if (unreadAmount > 0) {
        unreadAmount--;
    }
    chatCardElement->setUnreadAmount(unreadAmount == 0 ? "" : unreadAmount > 9 ? "⦿" : QString::number(unreadAmount));
}

void chatCardState::loadData() {
    if (chatId == "new") {
        title = "Добавить чат";
        isBanned = false;
        isBannedByCompanion = false;
        latestMessageValue = "Необходимы IP-адрес и порт для подключения";
        iconName = "add";
    } else if (!chatId.isEmpty()) {
        if (!dbConnection->open()) {
            qDebug() << "Error opening DB (chatCardState):" << dbConnection->lastError().text();
            return;
        }

        QSqlQuery query;
        query.prepare("SELECT title, icon, aesKey, iv, ipAddress, port, isBanned, isBannedByCompanion FROM Chats WHERE uuid = :chatId");
        query.bindValue(":chatId", chatId);

        if (!query.exec()) {
            qDebug() << "Error querying chat data (chatCardState):" << query.lastError().text();
            dbConnection->close();
            return;
        }

        if (query.next()) {
            QByteArray encryptedChatAESKey = query.value("aesKey").toByteArray();
            QByteArray iv = query.value("iv").toByteArray();
            QByteArray encryptedTitle = query.value("title").toByteArray();
            QByteArray encryptedIcon = query.value("icon").toByteArray();
            QByteArray encryptedIpAddress = query.value("ipAddress").toByteArray();
            QByteArray encryptedPortString = query.value("port").toByteArray();
            QByteArray encryptedIsBanned = query.value("isBanned").toByteArray();
            QByteArray encryptedIsBannedByCompanion = query.value("isBannedByCompanion").toByteArray();

            QString decryptedChatAesKeyString = decryptQStringAES(encryptedChatAESKey, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
            chatAesKey = aesKeyFromString(decryptedChatAesKeyString);

            title = decryptQStringAES(encryptedTitle, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
            iconName = decryptQStringAES(encryptedIcon, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
            ipAddress = decryptQStringAES(encryptedIpAddress, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
            portString = decryptQStringAES(encryptedPortString, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
            port = portString.toInt();
            isBanned = decryptQStringAES(encryptedIsBanned, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data())) == "true" ? true : false;
            isBannedByCompanion = decryptQStringAES(encryptedIsBannedByCompanion, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data())) == "true" ? true : false;
            qDebug() << isBanned << isBannedByCompanion;

            QString fetchLastMessageQuery = "SELECT uuid FROM Messages WHERE chatId = ? ORDER BY datetime(timestamp) DESC LIMIT 1;";
            QSqlQuery query;

            query.prepare(fetchLastMessageQuery);
            query.addBindValue(chatId);

            QString lastMessageUuid;

            if (query.exec() && query.next()) {
                lastMessageUuid = query.value(0).toString();
            } else {
                qDebug() << "Error executing query (fetchLastMessage):" << query.lastError().text();
            }

            QString fetchUnreadMessagesCountQuery = "SELECT COUNT(*) FROM Messages WHERE chatId = ? AND state = 'sent' AND sender != ?";
            QSqlQuery unreadQuery;
            query.prepare(fetchUnreadMessagesCountQuery);
            query.addBindValue(chatId);
            query.addBindValue(accountId);

            unreadQuery.prepare(fetchLastMessageQuery);
            unreadQuery.addBindValue(chatId);

            if (query.exec()) {
                if (query.next()) {
                    unreadAmount = query.value(0).toInt();
                } else {
                    qDebug() << "No unread messages found.";
                }
            } else {
                qDebug() << "Error executing query:" << query.lastError().text();
            }

            dbConnection->close();

            if (!lastMessageUuid.isEmpty()) {
                messageState* messageStateObject = new messageState("", aesKeyFromPassword, chatId, lastMessageUuid, dbConnection);
                setLatestMessage(messageStateObject->valueString, messageStateObject->timestampString, messageStateObject->state);
                delete messageStateObject;
            } else {
                setLatestMessage("Чат создан", "", "");
            }
        } else {
            qDebug() << "Chat with id" << chatId << "not found.";
        }
    }
}
