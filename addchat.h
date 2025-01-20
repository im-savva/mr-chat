#ifndef ADDCHAT_H
#define ADDCHAT_H

#include <QDialog>

namespace Ui {
class addChat;
}

class addChat : public QDialog
{
    Q_OBJECT

public:
    explicit addChat(QWidget *parent = nullptr);
    ~addChat();

private slots:
    void on_cancelButton_clicked();
    void on_continueButton_clicked();

signals:
    void chatAdded(const QString& connectionAdress);

private:
    Ui::addChat *ui;
};

#endif // ADDCHAT_H
