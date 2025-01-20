#include "useraccount.h"
#include "ui_useraccount.h"
#include "QSvgWidget.h"

userAccount::userAccount(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::userAccount)
{
    ui->setupUi(this);
}

void userAccount::setData(const QString& title, const QString& description) {
    if (title.isEmpty()) {
        ui->title->hide();
    } else {
        ui->title->setText(title);
    }
    if (description.isEmpty()) {
        ui->description->hide();
    } else {
        ui->description->setText(description);
    }
}

void userAccount::setIcon(const QString& svgPath) {
    QSvgWidget *svgWidget = new QSvgWidget(svgPath.isEmpty() ? ":/icons/profile/grinning_face_with_big_eyes.svg" : ":/icons/profile/" + svgPath + ".svg", this);
    svgWidget->setFixedSize(27, 27);
    svgWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    int svgX = (ui->chatIconContainer->width() - svgWidget->width()) / 2;
    int svgY = (ui->chatIconContainer->height() - svgWidget->height()) / 2;
    svgWidget->move(svgX, svgY);

    ui->userAccountRootLayout->insertWidget(0, svgWidget);
    svgWidget->setParent(ui->chatIconContainer);
}

userAccount::~userAccount()
{
    delete ui;
}
