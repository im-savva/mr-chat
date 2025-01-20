#ifndef ACCOUNTSETTINGS_H
#define ACCOUNTSETTINGS_H

#include <QDialog>

namespace Ui {
class accountSettings;
}

class accountSettings : public QDialog
{
    Q_OBJECT

public:
    explicit accountSettings(QWidget *parent = nullptr, const quint16& connectionPort_ = 0, const QString& nickname_ = "", const QString& iconName_ = "");
    ~accountSettings();
    void activateSaveButton();

signals:
    void accountIconSelected(const QString& iconName);
    void accountRemoved();
    void accountDataUpdated(const quint16& newConnectionPort, const QString& newNickname);
    void accountLogout();

private slots:
    void on_changeIconButton_clicked();

    void on_removeAccountButton_clicked();

    void on_saveButton_clicked();

    void on_logoutButton_clicked();

private:
    quint16 connectionPort;
    QString nickname, iconName;
    bool isPortValid = true, isNicknameValid = true;
    Ui::accountSettings *ui;
};

#endif // ACCOUNTSETTINGS_H
