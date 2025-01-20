#include "loginaccount.h"
#include "encryption.h"
#include "qtimer.h"
#include "ui_loginaccount.h"
#include "authvalidation.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QCryptographicHash>

logInAccount::logInAccount(QWidget *parent, QSqlDatabase *dbConnection_)
    : QDialog(parent)
    , ui(new Ui::logInAccount)
{

    dbConnection = dbConnection_;

    ui->setupUi(this);

    connect(ui->passwordInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto result = checkPassword(text);
        ui->passwordInputStatus->setText(result.second);
        if (result.first) {
            ui->continueButton->setEnabled(true);
        } else {
            ui->continueButton->setEnabled(false);
        }
    });

    ui->continueButton->setEnabled(false);
}

logInAccount::~logInAccount()
{
    delete ui;
}

void logInAccount::on_cancelButton_clicked()
{
    close();
    emit accountLogInCancelled();
}

void logInAccount::loadUserAccount(const QString& u_accountId) {
    accountId = u_accountId;

    if (!dbConnection->open()) {
        qDebug() << "Ошибка при открытии базы данных (loginAccount):" << dbConnection->lastError().text();
        return;
    }

    QString selectUserByIdQuery = QString("SELECT * FROM Users WHERE uuid = '%1';").arg(accountId);

    QSqlQuery query(selectUserByIdQuery);
    if (!query.exec()) {
        qDebug() << "Ошибка при выполнении запроса (loginAccount):" << query.lastError().text();
        dbConnection->close();
        return;
    }

    if (query.next()) {
        nickname = query.value("username").toString();
        encryptedPassword = query.value("password").toString();
    } else {
        dbConnection->close();
        return;
    }

    dbConnection->close();

    ui->logInAccountTitle->setText("С возвращением, " + nickname + "!");

    QTimer::singleShot(3, this, [=]() {
        QString text("С возвращением, " + nickname + "!");
        QFontMetrics metrics(ui->logInAccountTitle->font());
        QString elidedText = metrics.elidedText(text, Qt::ElideRight, ui->logInAccountTitle->width());
        ui->logInAccountTitle->setText(elidedText);
    });
}


void logInAccount::on_continueButton_clicked()
{
    if (encryptedPassword == QCryptographicHash::hash(ui->passwordInput->text().toUtf8(), QCryptographicHash::Sha256).toHex()) {
        close();
        AES_KEY *aesKeyFromPassword = generateAESKeyFromPassword(ui->passwordInput->text());
        emit accountLoggedIn(accountId, aesKeyFromPassword);
    } else {
        ui->passwordInputStatus->setText("Вы ввели неверный пароль. Попробуйте еще раз.");
        ui->continueButton->setEnabled(false);
    }
}

