#include "removechatapproval.h"
#include "qtimer.h"
#include "ui_removechatapproval.h"

removeChatApproval::removeChatApproval(QWidget *parent, const QString& chatName)
    : QDialog(parent)
    , ui(new Ui::removeChatApproval)
{
    ui->setupUi(this);

    ui->removeChatRequestTitle->setText("Удалить чат \"" + chatName+ "\"?");

    QTimer::singleShot(3, this, [=]() {
        QString text("Удалить чат \"" + chatName+ "\"?");
        QFontMetrics metrics(ui->removeChatRequestTitle->font());
        QString elidedText = metrics.elidedText(text, Qt::ElideRight, ui->removeChatRequestTitle->width());
        ui->removeChatRequestTitle->setText(elidedText);
    });
}

removeChatApproval::~removeChatApproval()
{
    delete ui;
}

void removeChatApproval::on_cancelButton_clicked()
{
    close();
}




void removeChatApproval::on_removeButton_clicked()
{
    emit chatRemoveApproved();
    close();
}

