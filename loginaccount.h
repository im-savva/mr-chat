#ifndef LOGINACCOUNT_H
#define LOGINACCOUNT_H

#include "openssl/aes.h"
#include "qsqldatabase.h"
#include <QDialog>

namespace Ui {
class logInAccount;
}

class logInAccount : public QDialog
{
    Q_OBJECT

public:
    explicit logInAccount(QWidget *parent = nullptr, QSqlDatabase *dbConnection_ = nullptr);
    ~logInAccount();
    void loadUserAccount(const QString& u_accountId);

signals:
    void accountLogInCancelled();
    void accountLoggedIn(const QString& accountId, AES_KEY *aesKeyFromPassword);

private slots:
    void on_cancelButton_clicked();

    void on_continueButton_clicked();

private:
    QSqlDatabase* dbConnection;
    QString nickname, encryptedPassword, accountId;
    Ui::logInAccount *ui;    
};

#endif // LOGINACCOUNT_H
