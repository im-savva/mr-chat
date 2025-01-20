#include "accountsettings.h"
#include "authvalidation.h"
#include "emojispicker.h"
#include "removeaccountapproval.h"
#include "ui_accountsettings.h"

accountSettings::accountSettings(QWidget *parent, const quint16& connectionPort_, const QString& nickname_, const QString& iconName_)
    : QDialog(parent)
    , ui(new Ui::accountSettings)
{
    ui->setupUi(this);

    iconName = iconName_;
    nickname = nickname_;
    connectionPort = connectionPort_;

    ui->nicknameInput->setText(nickname);
    ui->portInput->setText(QString::number(connectionPort));

    ui->saveButton->setEnabled(false);

    connect(ui->portInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto result = checkPort(text);
        ui->portInputStatus->setText(result.second);
        if (result.first) {
            isPortValid = true;
        } else {
            isPortValid = false;
        }
        activateSaveButton();
    });

    connect(ui->nicknameInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto result = checkNickname(text);
        ui->nicknameInputStatus->setText(result.second);
        if (result.first) {
            isNicknameValid = true;
        } else {
            isNicknameValid = false;
        }
        activateSaveButton();
    });
}

void accountSettings::activateSaveButton() {
    if (isPortValid && isNicknameValid) {
        ui->saveButton->setEnabled(true);
    } else {
        ui->saveButton->setEnabled(false);
    }
}

accountSettings::~accountSettings()
{
    delete ui;
}

void accountSettings::on_changeIconButton_clicked()
{
    emojisPicker *emojisPickerDialog = new emojisPicker(this, "MrChat. Выбор иконки профиля");
    emojisPickerDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(emojisPickerDialog, &emojisPicker::emojiPicked, this, [this](const QString& newIconName){
        emit accountIconSelected(newIconName);
        iconName = newIconName;
    });
    emojisPickerDialog->exec();
}

void accountSettings::on_removeAccountButton_clicked()
{
    removeAccountApproval *accountRemoveDialog = new removeAccountApproval(this, nickname);
    accountRemoveDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(accountRemoveDialog, &removeAccountApproval::accountRemoveApproved, this, [this](){
        close();
        emit accountRemoved();
    });
    accountRemoveDialog->exec();
}


void accountSettings::on_saveButton_clicked()
{
    close();
    emit accountDataUpdated(ui->portInput->text().toInt(), ui->nicknameInput->text());
}


void accountSettings::on_logoutButton_clicked()
{
    close();
    emit accountLogout();
}

