#ifndef FILEMESSAGE_H
#define FILEMESSAGE_H

#include "QSvgWidget.h"
#include <QWidget>

namespace Ui {
class fileMessage;
}

class fileMessage : public QWidget
{
    Q_OBJECT

public:
    explicit fileMessage(QWidget *parent = nullptr);
    ~fileMessage();
    void setData(const QString& fileName, const qint64& fileSize, const QString& messageTime, const QString& filterSubstring, const bool& isFromCompanion = false);
    void setState(const QString& messageState = "init");

signals:
    void fileMessageClicked();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Ui::fileMessage *ui;
    QSvgWidget *svgStateWidget = nullptr;
};

#endif // FILEMESSAGE_H
