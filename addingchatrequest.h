#ifndef ADDINGCHATREQUEST_H
#define ADDINGCHATREQUEST_H

#include <QDialog>

namespace Ui {
class addingChatRequest;
}

class addingChatRequest : public QDialog
{
    Q_OBJECT

public:
    explicit addingChatRequest(QWidget *parent = nullptr);
    ~addingChatRequest();

private:
    Ui::addingChatRequest *ui;
};

#endif // ADDINGCHATREQUEST_H
