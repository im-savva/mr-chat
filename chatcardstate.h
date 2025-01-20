#ifndef CHATCARDSTATE_H
#define CHATCARDSTATE_H
#include "chatcard.h"
#include "openssl/aes.h"
#include "qjsonobject.h"
#include "qsqldatabase.h"
#include <QWidget>
#include <QObject>

class chatCardState
{
public:
    quint16 index = 0;
    AES_KEY* chatAesKey;
    QString chatId = "", title, iconName, ipAddress = "", portString = "";
    bool isBanned, isBannedByCompanion, isActive = false, isOnline = false;
    quint16 port = 0, unreadAmount = 0;
    QString latestMessageValue = "";
    QString latestMessageState = "";
    QString latestMessageTimestampString = "";
    chatCard* chatCardElement = nullptr;
    chatCardState(const QString& accountId_, QString chatId_, AES_KEY *aesKeyFromPassword_, QSqlDatabase *dbConnection_);
    void setOnlineStatus(const bool& onlineStatus);
    void setNewIcon(const QString& newIconName);
    void setNewData(const QString& newIpAddress, const quint16& newPort, const QString& newNickname);
    void setChatBanned(const bool& isBanned_, const bool& isBannedByCompanion_);
    void setActiveStatus(const bool& activeStatus);
    void setLatestMessage(const QString& value, const QString& timestampString, const QString& state);
    void updateLatestMessage();
    void increaseUnreadAmout();
    void decreaseUnreadAmout();
    chatCard* render();
    void setOnlineStatus();

private:
    QSqlDatabase* dbConnection;
    AES_KEY* aesKeyFromPassword;
    QString accountId;
    void loadData();
};

#endif // CHATCARDSTATE_H
