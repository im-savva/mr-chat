#ifndef STICKERMESSAGE_H
#define STICKERMESSAGE_H

#include "QSvgWidget.h"
#include <QWidget>

namespace Ui {
class stickerMessage;
}

class stickerMessage : public QWidget
{
    Q_OBJECT

public:
    explicit stickerMessage(QWidget *parent = nullptr);
    void setData(const QString& stickerName, const QString& messageTime, const bool& isFromCompanion = false);
    void setState(const QString& messageState = "init");
    ~stickerMessage();

private:
    Ui::stickerMessage *ui;
    QSvgWidget *svgStateWidget = nullptr;
};

#endif // STICKERMESSAGE_H
