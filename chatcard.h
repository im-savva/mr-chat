#ifndef CHATCARD_H
#define CHATCARD_H

#include "QSvgWidget.h"
#include <QWidget>

namespace Ui {
class chatCard;
}

class chatCard : public QWidget
{
    Q_OBJECT

public:
    explicit chatCard(QWidget *parent = nullptr);
    ~chatCard();
    void setIcon(const QString& svgPath = "");
    void setTitle(const QString& title);
    void setId(const QString& id);
    void setLatestMessage(const QString& value = "", const QString& timestampString = "", const QString& state = "");
    void setUnreadAmount(const QString& unreadAmount = "");
    void setIsOnline(const bool& isOnline);

signals:
    void chatCardClicked();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QSvgWidget *svgStateWidget = nullptr;
    Ui::chatCard *ui;
};

#endif // CHATCARD_H
