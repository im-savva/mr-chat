#ifndef ADDACCOUNT_H
#define ADDACCOUNT_H

#include "openssl/aes.h"
#include "qsqldatabase.h"
#include <QDialog>

namespace Ui {
class addAccount;
}

class addAccount : public QDialog
{
    Q_OBJECT

public:
    explicit addAccount(QWidget *parent = nullptr, QSqlDatabase *dbConnection_ = nullptr);
    ~addAccount();

private slots:
    void on_cancelButton_clicked();

    void on_continueButton_clicked();

signals:
    void accountCreationCancelled();
    void accountCreated(const QString& accountId, AES_KEY *aesKeyFromPassword);

private:
    Ui::addAccount *ui;
    QSqlDatabase* dbConnection;
    void activateContinueButton();
    bool isNicknameCorrect = false;
    bool isPortCorrect = false;
    bool isPasswordCorrect = false;
};

#endif // ADDACCOUNT_H
