ifndef RECEIVEDMESSAGE_H
#define RECEIVEDMESSAGE_H

#include <QWidget>

namespace Ui {
class receivedMessage;
}

class receivedMessage : public QWidget
{
    Q_OBJECT

public:
    explicit receivedMessage(QWidget *parent = nullptr);
    ~receivedMessage();

private:
    Ui::receivedMessage *ui;
};

#endif // RECEIVEDMESSAGE_H
