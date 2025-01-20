#include "addingchatrequest.h"
#include "ui_addingchatrequest.h"

addingChatRequest::addingChatRequest(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::addingChatRequest)
{
    ui->setupUi(this);
}

addingChatRequest::~addingChatRequest()
{
    delete ui;
}
