#include "receivedmessage.h"
#include "ui_receivedmessage.h"

receivedMessage::receivedMessage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::receivedMessage)
{
    ui->setupUi(this);
}

receivedMessage::~receivedMessage()
{
    delete ui;
}
