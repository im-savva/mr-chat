#ifndef LOGINWITHPASSWORD_H
#define LOGINWITHPASSWORD_H

#include <QDialog>

namespace Ui {
class loginWithPassword;
}

class loginWithPassword : public QDialog
{
    Q_OBJECT

public:
    explicit loginWithPassword(QWidget *parent = nullptr);
    ~loginWithPassword();

    std::pair<bool, QString> checkPassword(const QString& password);
    QByteArray encryptPassword(const QString& password);

signals:
    void passwordReady(const QByteArray& encryptedPassword);

private slots:
    void onContinueClicked();

private:
    Ui::loginWithPassword *ui;
};

#endif // LOGINWITHPASSWORD_H
