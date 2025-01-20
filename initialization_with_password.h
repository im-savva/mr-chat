#ifndef LOGINWITHPASSWORD_H
#define LOGINWITHPASSWORD_H

#include <QDialog>

namespace Ui {
class initialization_with_password;
}

class initialization_with_password : public QDialog
{
    Q_OBJECT

public:
    explicit initialization_with_password(QWidget *parent = nullptr);
    ~initialization_with_password();

    std::pair<bool, QString> checkPassword(const QString& password);
    QByteArray encryptPassword(const QString& password);

signals:
    void passwordReady(const QByteArray& encryptedPassword);

private slots:
    void onContinueClicked();

private:
    Ui::initialization_with_password *ui;
};

#endif // LOGINWITHPASSWORD_H
