#ifndef CHATSETTINGS_H
#define CHATSETTINGS_H

#include <QDialog>

namespace Ui {
class chatSettings;
}

class chatSettings : public QDialog
{
    Q_OBJECT

public:
    explicit chatSettings(QWidget *parent = nullptr, const QString& connectionAddress_ = "", const QString& nickname_ = "", const QString& iconName_ = "", const bool& isBanned_ = false);
    ~chatSettings();
    void activateSaveButton();

private slots:
    void on_changeIconButton_clicked();

    void on_removeChatButton_clicked();

    void on_banChatButton_clicked();

    void on_saveButton_clicked();

signals:
    void chatIconSelected(const QString& iconName);
    void chatBanned();
    void chatRemoved();
    void chatDataUpdated(const QString& newConnectionAddress, const QString& newNickname);

private:
    QString connectionAddress, nickname, iconName;
    Ui::chatSettings *ui;
    bool isAddressValid = true, isNicknameValid = true;
};

#endif // CHATSETTINGS_H
