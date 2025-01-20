#ifndef USERACCOUNT_H
#define USERACCOUNT_H

#include <QWidget>

namespace Ui {
class userAccount;
}

class userAccount : public QWidget
{
    Q_OBJECT

public:
    explicit userAccount(QWidget *parent = nullptr);
    void setData(const QString& title, const QString& description = "");
    void setIcon(const QString& svgCode = "");
    ~userAccount();

private:
    Ui::userAccount *ui;
};

#endif // USERACCOUNT_H
