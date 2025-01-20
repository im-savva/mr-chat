#ifndef MESSAGE_H
#define MESSAGE_H

#include "QSvgWidget.h"
#include <QWidget>

namespace Ui {
class message;
}

class message : public QWidget
{
    Q_OBJECT

public:
    explicit message(QWidget *parent = nullptr);
    ~message();
    void setData(const QString& messageText, const QString& messageTime, const QString& filterSubstring = "", const bool& isFromCompanion = false);
    void setState(const QString& messageState = "init");
    void adjustSize();

private:
    Ui::message *ui;
    QSvgWidget *svgStateWidget = nullptr;
};

#endif // MESSAGE_H
