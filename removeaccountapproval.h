#ifndef REMOVEACCOUNTAPPROVAL_H
#define REMOVEACCOUNTAPPROVAL_H

#include <QDialog>

namespace Ui {
class removeAccountApproval;
}

class removeAccountApproval : public QDialog
{
    Q_OBJECT

public:
    explicit removeAccountApproval(QWidget *parent = nullptr, const QString& chatName = "");
    ~removeAccountApproval();

private slots:
    void on_cancelButton_clicked();

    void on_removeButton_clicked();

signals:
    void accountRemoveApproved();

private:
    Ui::removeAccountApproval *ui;
};

#endif // REMOVEACCOUNTAPPROVAL_H
