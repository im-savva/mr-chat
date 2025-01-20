#ifndef REMOVECHATAPPROVAL_H
#define REMOVECHATAPPROVAL_H

#include <QDialog>

namespace Ui {
class removeChatApproval;
}

class removeChatApproval : public QDialog
{
    Q_OBJECT

public:
    explicit removeChatApproval(QWidget *parent = nullptr, const QString& chatName = "");
    ~removeChatApproval();

private slots:
    void on_cancelButton_clicked();

    void on_removeButton_clicked();

signals:
    void chatRemoveApproved();

private:
    Ui::removeChatApproval *ui;
};

#endif // REMOVECHATAPPROVAL_H
