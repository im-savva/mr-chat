#include "stickermessage.h"
#include "ui_stickermessage.h"

stickerMessage::stickerMessage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::stickerMessage)
{
    ui->setupUi(this);
}

stickerMessage::~stickerMessage()
{
    delete ui;
}

void stickerMessage::setData(const QString& stickerName, const QString& messageTime, const bool& isFromCompanion) {
    ui->messageTime->setText(messageTime);

    if (isFromCompanion) {
        layout()->removeItem(ui->leftSpacer);
    } else {
        layout()->removeItem(ui->rightSpacer);
    }

    QSvgWidget *svgWidget = new QSvgWidget(":/icons/profile/" + stickerName + ".svg", this);
    svgWidget->setFixedSize(200, 200);
    svgWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    ui->stickerMessageBoxLayout->insertWidget(0, svgWidget);
}

void stickerMessage::setState(const QString& messageState) {
    float svgWidgetAspectRatio = 74.46 / 122.88;
    int svgWidgetSize = 14;

    if (svgStateWidget != nullptr) {
        delete svgStateWidget;
    }

    svgStateWidget = new QSvgWidget(":/icons/message/state-" + messageState + ".svg", this);
    svgStateWidget->setFixedSize(svgWidgetSize, svgWidgetAspectRatio * svgWidgetSize);
    svgStateWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    ui->messageDetailsLayout->insertWidget(1, svgStateWidget);
}
