#include "emojispicker.h"
#include "qpushbutton.h"
#include "ui_emojispicker.h"
#include <QPixmap>

emojisPicker::emojisPicker(QWidget *parent, const QString& windowTitle)
    : QDialog(parent)
    , ui(new Ui::emojisPicker)
{
    ui->setupUi(this);
    setWindowTitle(windowTitle);

    int buttonIndex = 0;

    for (const auto& pair : emojiNames) {
        QPushButton *emojiButton = new QPushButton(this);
        emojiButton->setFixedSize(51, 51);
        emojiButton->setObjectName("emojiPickerButton");
        emojiButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        emojiButton->setCursor(Qt::PointingHandCursor);

        QString emojiFileName = QString::fromStdString(pair.first);

        QPixmap icon(":/icons/profile/" + emojiFileName + ".svg");
        icon = icon.scaled(QSize(500, 500), Qt::KeepAspectRatio);

        emojiButton->setIconSize(QSize(30, 30));
        emojiButton->setIcon(icon);

        ui->emojisContainerLayout->addWidget(emojiButton, buttonIndex / 6, buttonIndex % 6);

        connect(emojiButton, &QPushButton::clicked, this, [emojiFileName, this]() {
            emit emojiPicked(emojiFileName);
            close();
        });

        buttonIndex++;
    }
}

emojisPicker::~emojisPicker()
{
    delete ui;
}
