#ifndef ADDCHATREQUEST_H
#define ADDCHATREQUEST_H

#include "qdatetime.h"
#include <QDialog>

namespace Ui {
class addChatRequest;
}

class addChatRequest : public QDialog
{
    Q_OBJECT

public:
    explicit addChatRequest(QWidget *parent = nullptr);
    void setContent(const QString& expireTimestamp, const QString& nickname, const QString& connectionIpAddress_, const quint16& connectionPort_, const QString& connectionChatId_, const QString& rsaPublicKey_);
    ~addChatRequest();

private slots:
    void on_banButton_clicked();
    void on_continueButton_clicked();

signals:
    void requestAccepted(const QString& connectionNickname, const QString& connectionIpAddress, const quint16& connectionPort, const QString& connectionChatId, const QString& rsaPublicKey);
    void requestBanned(const QString& connectionNickname, const QString& connectionIpAddress, const quint16& connectionPort, const QString& connectionChatId, const QString& rsaPublicKey);
    void requestRejected(const QString& connectionNickname, const QString& connectionIpAddress, const quint16& connectionPort, const QString& connectionChatId, const QString& rsaPublicKey);

private:
    Ui::addChatRequest *ui;
    QDateTime currentExpirationTime;
    QString connectionChatId, connectionIpAddress, connectionNickname, rsaPublicKey;
    quint16 connectionPort;
    bool isClosedProgrammatically = false;
    void closeProgrammatically();

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // ADDCHATREQUEST_H
