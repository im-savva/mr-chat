#include "accountchoice.h"
#include "qdatetime.h"
#include "qsqlerror.h"
#include "ui_accountchoice.h"
#include "useraccount.h"
#include <QSqlDatabase>
#include <QSqlQuery>

accountChoice::accountChoice(QWidget *parent, QSqlDatabase *dbConnection_)
    : QDialog(parent)
    , ui(new Ui::accountChoice)
{

    dbConnection = dbConnection_;

    ui->setupUi(this);

    if (!dbConnection->open()) {
        qDebug() << "Ошибка при открытии базы данных (accountChoice):" << dbConnection->lastError().text();
        return;
    }

    QStringList tables = dbConnection->tables();
    if (!tables.contains("Users")) {
        QMetaObject::invokeMethod(this, "emitAccountChosen", Qt::QueuedConnection, Q_ARG(QString, "new"));
        return;
    } else {
        QString selectUsersQuery = "SELECT * FROM Users;";

        QSqlQuery query(selectUsersQuery);
        if (!query.exec()) {
            qDebug() << "Ошибка при выполнении запроса (accountChoice):" << query.lastError().text();
            dbConnection->close();
            return;
        }
        qint64 usersAmount = 0;
        while (query.next()) {
            ++usersAmount;
            QString username = query.value("username").toString();
            QString id = query.value("uuid").toString();
            QString lastLoginDateString = query.value("lastLoginDate").toString();
            QString iconName = query.value("iconName").toString();

            QDateTime currentDateTime = QDateTime::currentDateTime();
            QDateTime lastLoginDateTime = QDateTime::fromString(lastLoginDateString, Qt::ISODate);
            QDateTime::currentDateTime();
            QDate currentDate = currentDateTime.date();
            QDate lastLoginDate = lastLoginDateTime.date();
            qint64 daysPassed = lastLoginDate.daysTo(currentDate);

            QString description;
            if (daysPassed == 0) {
                description = "Сегодня";
            } else if (daysPassed == 1) {
                description = "Вчера";
            } else {
                if (daysPassed == 1 || (daysPassed % 10 == 1 && daysPassed != 11)) {
                    description = QString::number(daysPassed) + " день назад";
                } else if ((daysPassed >= 2 && daysPassed <= 4) || ((daysPassed % 10 >= 2 && daysPassed % 10 <= 4) && (daysPassed < 10 || daysPassed > 20))) {
                    description = QString::number(daysPassed) + " дня назад";
                } else {
                    description = QString ::number(daysPassed) + " дней назад";
                }
            }

            renderUserAccountCard(username, description, iconName, id);
        }

        dbConnection->close();

        if (usersAmount < 3) {
            renderUserAccountCard("Добавить аккаунт", "", "add", "new");
        }

        connect(ui->accountChoicesList, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
            close();
            QString id = item->data(Qt::UserRole).toString();
            emit accountChosen(id);
        });
    }
}

void accountChoice::renderUserAccountCard(const QString& accountTitle, const QString& accountDescription = "", const QString& accountIconPath = "", const QString& accountId = "") {
    userAccount *userAccountCard = new class userAccount(this);
    userAccountCard->setData(accountTitle, accountDescription);
    userAccountCard->setIcon(accountIconPath);
    QListWidgetItem *cardItem = new QListWidgetItem(ui->accountChoicesList);
    cardItem->setSizeHint(QSize(300, 64));
    cardItem->setData(Qt::UserRole, accountId);
    ui->accountChoicesList->addItem(cardItem);
    ui->accountChoicesList->setItemWidget(cardItem, userAccountCard);
}

accountChoice::~accountChoice()
{
    delete ui;
}
