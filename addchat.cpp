#include "addchat.h"
#include "authvalidation.h"
#include "ui_addchat.h"

addChat::addChat(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::addChat)
{
    ui->setupUi(this);

    connect(ui->connectionAdressInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto result = checkConnectionAdress(text);
        ui->connectionAdressInputStatus->setText(result.second);
        if (result.first) {
            ui->continueButton->setEnabled(true);
        } else {
            ui->continueButton->setEnabled(false);
        }
    });

    ui->continueButton->setEnabled(false);
}

addChat::~addChat()
{
    delete ui;
}

void addChat::on_cancelButton_clicked()
{
    close();
}


void addChat::on_continueButton_clicked()
{
    close();
    emit chatAdded(ui->connectionAdressInput->text());
}
