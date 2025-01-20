#include "removeaccountapproval.h"
#include "qtimer.h"
#include "ui_removeaccountapproval.h"

removeAccountApproval::removeAccountApproval(QWidget *parent, const QString& nickname)
    : QDialog(parent)
    , ui(new Ui::removeAccountApproval)
{
    ui->setupUi(this);

    ui->removeAccountTitle->setText("Удалить аккаунт \"" + nickname+ "\"?");

    QTimer::singleShot(3, this, [=]() {
        QString text("Удалить аккаунт \"" + nickname+ "\"?");
        QFontMetrics metrics(ui->removeAccountTitle->font());
        QString elidedText = metrics.elidedText(text, Qt::ElideRight, ui->removeAccountTitle->width());
        ui->removeAccountTitle->setText(elidedText);
    });
}

removeAccountApproval::~removeAccountApproval()
{
    delete ui;
}

void removeAccountApproval::on_cancelButton_clicked()
{
    close();
}


void removeAccountApproval::on_removeButton_clicked()
{

    emit accountRemoveApproved();
    close();
}

