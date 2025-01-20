#ifndef ACCOUNTCHOICE_H
#define ACCOUNTCHOICE_H

#include "qsqldatabase.h"
#include <QDialog>

namespace Ui {
class accountChoice;
}

class accountChoice : public QDialog
{
    Q_OBJECT

public:
    explicit accountChoice(QWidget *parent = nullptr, QSqlDatabase* dbConnection_ = nullptr);
    void renderUserAccountCard(const QString& accountTitle, const QString& accountDescription, const QString& accountIconPath, const QString& accountId);
    ~accountChoice();

signals:
    void accountChosen(const QString& accountId);

private slots:
    void emitAccountChosen(const QString& arg) {
        close();
        emit accountChosen(arg);
    }

private:
    Ui::accountChoice *ui;
    QSqlDatabase* dbConnection;
};

#endif // ACCOUNTCHOICE_H
