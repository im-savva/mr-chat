#include "chatsettings.h"
#include "authvalidation.h"
#include "emojispicker.h"
#include "removechatapproval.h"
#include "ui_chatsettings.h"

chatSettings::chatSettings(QWidget *parent, const QString& connectionAddress_, const QString& nickname_, const QString& iconName_, const bool& isBanned_)
    : QDialog(parent)
    , ui(new Ui::chatSettings)
{
    ui->setupUi(this);

    iconName = iconName_;
    nickname = nickname_;
    connectionAddress = connectionAddress_;

    if (isBanned_) {
        ui->banChatButton->setText("Разблокировать собеседника");
    }

    ui->nicknameInput->setText(nickname);
    ui->addressInput->setText(connectionAddress);

    ui->saveButton->setEnabled(false);

    connect(ui->addressInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto result = checkConnectionAdress(text);
        ui->addressInputStatus->setText(result.second);
        if (result.first) {
            isAddressValid = true;
        } else {
            isAddressValid = false;
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

void chatSettings::activateSaveButton() {
    if (isAddressValid && isNicknameValid) {
        ui->saveButton->setEnabled(true);
    } else {
        ui->saveButton->setEnabled(false);
    }
}

chatSettings::~chatSettings()
{
    delete ui;
}

void chatSettings::on_changeIconButton_clicked()
{
    emojisPicker *emojisPickerDialog = new emojisPicker(this, "MrChat. Выбор иконки чата");
    emojisPickerDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(emojisPickerDialog, &emojisPicker::emojiPicked, this, [this](const QString& newIconName){
        emit chatIconSelected(newIconName);
        iconName = newIconName;
    });
    emojisPickerDialog->exec();
}

void chatSettings::on_removeChatButton_clicked()
{
    removeChatApproval *chatRemoveDialog = new removeChatApproval(this, nickname);
    chatRemoveDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(chatRemoveDialog, &removeChatApproval::chatRemoveApproved, this, [this](){
        emit chatRemoved();
        close();
    });
    chatRemoveDialog->exec();
}


void chatSettings::on_banChatButton_clicked()
{
    emit chatBanned();
    close();
}


void chatSettings::on_saveButton_clicked()
{
    emit chatDataUpdated(ui->addressInput->text(), ui->nicknameInput->text());
    close();
}

