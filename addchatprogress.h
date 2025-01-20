#ifndef ADDCHATPROGRESS_H
#define ADDCHATPROGRESS_H

#include "peerinitialconnection.h"
#include "quuid.h"
#include <QDialog>
#include <openssl/rsa.h>

namespace Ui {
class addchatprogress;
}

class addchatprogress : public QDialog
{
    Q_OBJECT

public:
    explicit addchatprogress(QWidget *parent = nullptr);
    ~addchatprogress();
    void setContext(const QString& ipAddress_, const quint16& port_, const QString& ipAddress, const quint16& port, const QString& nickname);

signals:
    void chatAdded(const QString& newChatUuid, const QString& newChatTitle, const QString& newChatAesKey, const QString& newChatIpAddress, const quint16& newChatPort, bool isBanned = false, bool isBannedByCompanion = false, bool isFromCompanion = false);

private:
    void setContent(const QString& title, const QString& description, const qint16& progress);
    Ui::addchatprogress *ui;
    PeerInitialConnection peerInitialConnection;
    void proccessInitialConnectionResponse(const QJsonDocument& connectionDetails);
    const QString chatId = QUuid::createUuid().toString().mid(1, 36);
    bool showTimeoutMessage = true;
    void shutDownAddingChatProgress();
    RSA *rsaKeyPair;
};

#endif // ADDCHATPROGRESS_H
