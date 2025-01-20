#include "messagestate.h"
#include "emojispicker.h"
#include "encryption.h"
#include "filemessage.h"
#include "message.h"
#include "qjsondocument.h"
#include "qsqlerror.h"
#include "qsqlquery.h"
#include "qtimezone.h"
#include "stickermessage.h"

messageState::messageState(const QString& accountId_, AES_KEY *aesKeyFromPassword_, const QString& chatId_, const QString& messageId_, QSqlDatabase *dbConnection_) {
    accountId = accountId_;
    chatId = chatId_;
    dbConnection = dbConnection_;
    messageId = messageId_;
    aesKeyFromPassword = aesKeyFromPassword_;
    loadData();
}

messageState::~messageState() {
    if (messageElement != nullptr) {
        delete messageElement;
    }
}

bool isValidUUID(const QString &uuidString) {
    QRegularExpression regex("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
    return regex.match(uuidString).hasMatch();
}

QWidget* messageState::render(const QString& filterSubstring) {
    if (messageId.isEmpty()) {
        qDebug() << "Chat ID is empty. Chat card could not be rendered.";
        return nullptr;
    }
    if (messageElement != nullptr) {
        delete messageElement;
        messageElement = nullptr;
    }
    if (type == "text" || type == "timestamp" || type == "info" || type == "file" || type == "sticker" || type == "answer" || type == "question") {
        if (type == "text" || type == "timestamp" || type == "info" || type == "answer" || type == "question") {
            messageElement = new message();
        } else if (type == "file") {
            messageElement = new fileMessage();
        } else if (type == "sticker") {
            messageElement = new stickerMessage();
        }
        if (type == "timestamp" || type == "info") {
            messageElement->setObjectName("messageTimestamp");
        } else {
            if (isFromCompanion) {
                messageElement->setObjectName("messageFromCompanion");
            } else {
                messageElement->setObjectName("messageFromMe");
            }
        }
        if (type == "text" || type == "timestamp" || type == "info" || type == "answer" || type == "question") {
            dynamic_cast<message*>(messageElement)->setData(value["text"].toString(), type == "text" ? timestamp.toString("HH:mm") : type == "answer" || type == "question" ? "55:55" : "", filterSubstring, isFromCompanion);
            dynamic_cast<message*>(messageElement)->setState(state);
        } else if (type == "file") {
            dynamic_cast<fileMessage*>(messageElement)->setData(value["name"].toString(), fileSize, timestamp.toString("HH:mm"), filterSubstring, isFromCompanion);
            dynamic_cast<fileMessage*>(messageElement)->setState(state);
        } else if (type == "sticker") {
            dynamic_cast<stickerMessage*>(messageElement)->setData(value["name"].toString(), timestamp.toString("HH:mm"), isFromCompanion);
            dynamic_cast<stickerMessage*>(messageElement)->setState(state);
        }
    }
    else {
        qDebug() << "Render message: Unsupported message type";
    }
    return messageElement;
}

QString getRussianGenitiveMonth(const QString& englishMonth) {
    static const QMap<QString, QString> monthTranslations = {
        {"January", "января"},
        {"February", "февраля"},
        {"March", "марта"},
        {"April", "апреля"},
        {"May", "мая"},
        {"June", "июня"},
        {"July", "июля"},
        {"August", "августа"},
        {"September", "сентября"},
        {"October", "октября"},
        {"November", "ноября"},
        {"December", "декабря"}
    };

    return monthTranslations.value(englishMonth);
}

void messageState::adjustSize() {
    if (messageElement != nullptr) {
        if (type != "file" && type != "sticker") {
            dynamic_cast<message*>(messageElement)->adjustSize();
        }
    }
}

void messageState::updateState(const QString& newState) {

    if (((newState == "sent" || newState == "init") && state == "read") || (newState == "init" && state == "sent")) {
        return;
    }

    if (!dbConnection->open()) {
        qDebug() << "Error opening DB (chatCardState):" << dbConnection->lastError().text();
        return;
    }

    QSqlQuery query;
    query.prepare("UPDATE Messages SET state = :newState WHERE uuid = :messageId");
    query.bindValue(":newState", newState);
    query.bindValue(":messageId", messageId);

    if (!query.exec()) {
        qDebug() << "Error updating message state (chatCardState):" << query.lastError().text();
        dbConnection->close();
        return;
    }

    state = newState;

    if (messageElement != nullptr) {
        if (type == "text" || type == "timestamp" || type == "info") {
            dynamic_cast<message*>(messageElement)->setState(state);
        } else if (type == "file") {
            dynamic_cast<fileMessage*>(messageElement)->setState(state);
        } else if (type == "sticker") {
            dynamic_cast<stickerMessage*>(messageElement)->setState(state);
        }
    }

    dbConnection->close();
}

void messageState::loadData() {
    if (!isValidUUID(messageId)) {
        QStringList infoParts = messageId.split("=");

        if (infoParts.size() == 2) {
            QString infoType = infoParts.at(0);
            QString infoValue = infoParts.at(1);
            qDebug() << infoType << infoValue;
            if (infoType == "info") {
                type = "info";
                value["text"] = infoValue;
            } else if (infoType == "timestamp") {
                timestampString = infoValue;
                timestamp = QDateTime::fromString(timestampString, Qt::ISODate);
                type = "timestamp";
                value["text"] = timestamp.toString("d") + " " + getRussianGenitiveMonth(timestamp.toString("MMMM"));

                QDateTime currentTimestamp = QDateTime::currentDateTime();
                if (currentTimestamp.date().year() != timestamp.date().year()) {
                    value["text"] = timestamp.toString("dd.MM.yyyy");
                }
            } else if (infoType == "question") {
                type = "question";
                value["text"] = infoValue;
                isFromCompanion = false;
            } else if (infoType == "answer") {
                type = "answer";
                value["text"] = infoValue;
                isFromCompanion = true;
            }
        } else {
            qDebug() << "Invalid message id:" << messageId;
        }
    } else {
        if (!dbConnection->open()) {
            qDebug() << "Error opening DB (chatCardState):" << dbConnection->lastError().text();
            return;
        }

        QSqlQuery query;
        query.prepare("SELECT iv, type, timestamp, sender, value, state FROM Messages WHERE uuid = :messageId");
        query.bindValue(":messageId", messageId);

        if (!query.exec()) {
            qDebug() << "Error querying chat data (chatCardState):" << query.lastError().text();
            dbConnection->close();
            return;
        }

        if (query.next()) {
            QByteArray iv = query.value("iv").toByteArray();
            QByteArray encryptedType = query.value("type").toByteArray();
            timestampString = query.value("timestamp").toString();
            timestamp = QDateTime::fromString(timestampString, Qt::ISODate);
            timestamp.setTimeZone(QTimeZone("Europe/Moscow"));
            sender = query.value("sender").toString();
            QByteArray encryptedValue = query.value("value").toByteArray();
            state = query.value("state").toString();

            type = decryptQStringAES(encryptedType, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
            isFromCompanion = (sender != accountId);
            QString valueJSONString = decryptQStringAES(encryptedValue, aesKeyFromPassword, reinterpret_cast<unsigned char*>(iv.data()));
            value = QJsonDocument::fromJson(valueJSONString.toUtf8()).object();
            encryptedValue.clear();
            valueJSONString.clear();

            if (type == "text") {
                valueString = value["text"].toString();
            } else if (type == "file") {
                valueString = "Файл: " + value["name"].toString();
                fileSize = value["size"].toInt();
            } else if (type == "sticker") {
                QString stickerName = value["name"].toString();
                std::string stickerNameStd = stickerName.toStdString();

                auto it = emojiNames.find(stickerNameStd);
                if (it != emojiNames.end()) {
                    valueString = "Стикер: " + QString::fromStdString(it->second);
                } else {
                    valueString = "Неизвестный стикер";
                }
            }

        } else {
            qDebug() << "Message with id" << chatId << "not found.";
        }

        dbConnection->close();
    }
}
