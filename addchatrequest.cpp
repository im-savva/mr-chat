#include "addchatrequest.h"
#include "qdatetime.h"
#include "qpropertyanimation.h"
#include "qtimer.h"
#include "ui_addchatrequest.h"

addChatRequest::addChatRequest(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::addChatRequest)
{
    ui->setupUi(this);
}

addChatRequest::~addChatRequest()
{
    delete ui;
}

void addChatRequest::closeProgrammatically() {
    isClosedProgrammatically = true;
    close();
};

void addChatRequest::on_banButton_clicked()
{
    emit requestBanned(connectionNickname, connectionIpAddress, connectionPort, connectionChatId, rsaPublicKey);
    closeProgrammatically();
}

void addChatRequest::closeEvent(QCloseEvent *event)  {
    if (!isClosedProgrammatically) {
        emit requestRejected(connectionNickname, connectionIpAddress, connectionPort, connectionChatId, rsaPublicKey);
    }
    QDialog::closeEvent(event);
}


void addChatRequest::on_continueButton_clicked()
{
    emit requestAccepted(connectionNickname, connectionIpAddress, connectionPort, connectionChatId, rsaPublicKey);
    closeProgrammatically();
}

void addChatRequest::setContent(const QString& expireTimestamp, const QString& nickname_, const QString& connectionIpAddress_, const quint16& connectionPort_, const QString& connectionChatId_, const QString& rsaPublicKey_) {
    connectionChatId = connectionChatId_;
    connectionPort = connectionPort_;
    connectionNickname = nickname_;
    connectionIpAddress = connectionIpAddress_;
    rsaPublicKey = rsaPublicKey_;
    QString connectionPortString = QString::number(connectionPort);
    ui->addChatRequestTitle->setText("Новый чат \"" + nickname_ + "\"");
    ui->addChatRequestDescription->setText("Вам был отправлен запрос на добавление чата \"" + nickname_ + "\" от " + connectionIpAddress + ":" + connectionPortString + ".");
    currentExpirationTime = QDateTime::fromString(expireTimestamp, Qt::ISODate);
    QTimer::singleShot(60000, this, [this](){
        closeProgrammatically();
    });

    QPropertyAnimation *animation = new QPropertyAnimation(ui->requsetTimer, "value");
    animation->setDuration(60000);
    animation->setEasingCurve(QEasingCurve::InOutQuad);
    animation->setEndValue(0);
    animation->start();
}
