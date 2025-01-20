#include "addaccount.h"
#include "encryption.h"
#include "ui_addaccount.h"
#include "authvalidation.h"
#include <QUuid>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QCryptographicHash>

addAccount::addAccount(QWidget *parent, QSqlDatabase *dbConnection_)
    : QDialog(parent)
    , ui(new Ui::addAccount)
{

    dbConnection = dbConnection_;

    ui->setupUi(this);

    connect(ui->passwordInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto result = checkPassword(text);
        ui->passwordInputStatus->setText(result.second);
        if (result.first) {
            isPasswordCorrect = true;
            activateContinueButton();
        } else {
            isPasswordCorrect = false;
            activateContinueButton();
        }
    });
    connect(ui->nicknameInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto result = checkNickname(text);
        ui->nicknameInputStatus->setText(result.second);
        if (result.first) {
            isNicknameCorrect = true;
            activateContinueButton();
        } else {
            isNicknameCorrect = false;
            activateContinueButton();
        }
    });
    connect(ui->portInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto result = checkPort(text);
        ui->portInputStatus->setText(result.second);
        if (result.first) {
            isPortCorrect = true;
            activateContinueButton();
        } else {
            isPortCorrect = false;
            activateContinueButton();
        }
    });

    ui->continueButton->setEnabled(false);
}

addAccount::~addAccount()
{
    delete ui;
}

void addAccount::on_cancelButton_clicked()
{
    close();
    emit accountCreationCancelled();
}

void addAccount::activateContinueButton() {
    if (isPasswordCorrect && isNicknameCorrect && isPortCorrect) {
        ui->continueButton->setEnabled(true);
    } else {
        ui->continueButton->setEnabled(false);
    }
}

void addAccount::on_continueButton_clicked()
{

    if (!dbConnection->open()) {
        qDebug() << "Ошибка при открытии базы данных (addAccount):" << dbConnection->lastError().text();
        return;
    }

    QString createTableQuery = "CREATE TABLE IF NOT EXISTS Users ("
                               "uuid TEXT PRIMARY KEY,"
                               "username TEXT NOT NULL,"
                               "iconName TEXT NOT NULL,"
                               "password TEXT NOT NULL,"
                               "lastLoginDate TEXT,"
                               "port INTEGER UNIQUE,"
                               "UNIQUE(username)"
                               ");";

    QSqlQuery query;
    if (!query.exec(createTableQuery)) {
        qDebug() << "Ошибка при создании таблицы (addAccount):" << query.lastError().text();
        dbConnection->close();
        close();
        emit accountCreationCancelled();
        return;
    }

    QString nickname = ui->nicknameInput->text();
    QString port = ui->portInput->text();
    QString password = ui->passwordInput->text();
    QString encryptedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
    QString uuid = QUuid::createUuid().toString().mid(1, 36);
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString lastLoginDateTime = currentDateTime.toString(Qt::ISODate);

    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM Users WHERE port = :port");
    checkQuery.bindValue(":port", port);

    if (!checkQuery.exec()) {
        qDebug() << "Error executing query (addAccount):" << checkQuery.lastError().text();
    } else {
        if (checkQuery.next()) {
            int count = checkQuery.value(0).toInt();
            if (count > 0) {
                ui->portInputStatus->setText("Порт уже используется другим пользователем.");
                isPortCorrect = false;
                activateContinueButton();
            } else {
                QString insertUserQuery = "INSERT INTO Users (uuid, username, iconName, password, lastLoginDate, port) "
                                          "VALUES (:uuid, :nickname, :iconName, :encryptedPassword, :lastLoginDate, :port);";

                query.prepare(insertUserQuery);
                query.bindValue(":uuid", uuid);
                query.bindValue(":nickname", nickname);
                query.bindValue(":iconName", "grinning_face_with_big_eyes");
                query.bindValue(":encryptedPassword", encryptedPassword);
                query.bindValue(":lastLoginDate", lastLoginDateTime);
                query.bindValue(":port", port);

                if (!query.exec()) {
                    qDebug() << "Ошибка при добавлении пользователя (addAccount):" << query.lastError().text();
                    dbConnection->close();
                    close();
                    emit accountCreationCancelled();
                    return;
                }

                dbConnection->close();
                close();
                AES_KEY *aesKeyFromPassword = generateAESKeyFromPassword(password);
                emit accountCreated(uuid, aesKeyFromPassword);
            }
        }
    }
}
