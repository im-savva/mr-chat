#ifndef MESSAGESTATE_H
#define MESSAGESTATE_H

#include "message.h"
#include "openssl/aes.h"
#include "qjsonobject.h"
#include "qsqldatabase.h"
#include <QWidget>
#include <QObject>

class messageState
{
public:
    bool isFromCompanion = false;
    QString messageId;
    quint16 index = 0;
    qint64 fileSize = 0;
    QString type = "";
    QString state = "";
    QString timestampString = "";
    QDateTime timestamp;
    QString sender = "";
    QJsonObject value;
    QString valueString = "";
    QWidget* messageElement = nullptr;
    QWidget* render(const QString& filterSubstring = "");
    void updateState(const QString& newState);
    void adjustSize();
    messageState(const QString& accountId_, AES_KEY *aesKeyFromPassword_, const QString& chatId_, const QString& messageId_, QSqlDatabase *dbConnection_);
    ~messageState();

private:
    QString chatId, accountId;
    QSqlDatabase* dbConnection;
    AES_KEY* aesKeyFromPassword;
    void loadData();
};

#endif // MESSAGESTATE_H
