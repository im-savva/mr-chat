#include "chatcard.h"
#include "qdatetime.h"
#include "qtimer.h"
#include "ui_chatcard.h"
#include "QSvgWidget.h"
#include <QMouseEvent>

chatCard::chatCard(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::chatCard)
{
    ui->setupUi(this);
}

void chatCard::mouseReleaseEvent(QMouseEvent *event)
{
    // Crashes the app
    // QWidget::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton) {
        emit chatCardClicked();
    }
}

chatCard::~chatCard()
{
    if (svgStateWidget != nullptr) {
        delete svgStateWidget;
    }
    delete ui;
}

void chatCard::setTitle(const QString& title) {
    if (title.isEmpty()) {
        ui->chatTitle->hide();
    } else {
        ui->chatTitle->setText(title);
    }
}

void chatCard::setId(const QString& id) {
    setProperty("id", id);
}

bool isDateThisWeek(const QDateTime& dateTime) {
    QDate currentDate = QDate::currentDate();
    int currentWeekNumber = currentDate.weekNumber();
    int currentYear = currentDate.year();

    QDate date = dateTime.date();
    int weekNumber = date.weekNumber();
    int year = date.year();

    return (weekNumber == currentWeekNumber && year == currentYear);
}

bool isDateToday(const QDateTime& dateTime) {
    QDate currentDate = QDate::currentDate();
    QDate date = dateTime.date();
    return (date == currentDate);
}

QMap<QString, QString> dayTranslations = {
    {"Mon", "Пн"},
    {"Tue", "Вт"},
    {"Wed", "Ср"},
    {"Thu", "Чт"},
    {"Fri", "Пт"},
    {"Sat", "Сб"},
    {"Sun", "Вс"}
};

void chatCard::setLatestMessage(const QString& value, const QString& timestampString, const QString& state) {

    if (svgStateWidget != nullptr) {
        delete svgStateWidget;
    }

    if (value.isEmpty()) {
        ui->latestMessage->hide();
        ui->latestMessageDate->hide();
        return;
    }

    if (!timestampString.isEmpty()) {
        QDateTime timestamp = QDateTime::fromString(timestampString, Qt::ISODate);
        QString timestampFormattedString = "";
        if (!timestamp.isNull()) {
            if (isDateToday(timestamp)) {
                timestampFormattedString = timestamp.toString("HH:mm");
            } else if (isDateThisWeek(timestamp)) {
                timestampFormattedString = dayTranslations.value(timestamp.toString("ddd"));
            } else {
                timestampFormattedString = timestamp.toString("dd.MM.yyyy");
            }
        }
        ui->latestMessageDate->setText(timestampFormattedString);
    } else {
        ui->latestMessageDate->hide();
    }

    ui->latestMessage->setText(value);

    QTimer::singleShot(3, this, [=]() {
        QString text(value);
        QFontMetrics metrics(ui->latestMessage->font());
        QString elidedText = metrics.elidedText(text, Qt::ElideRight, ui->latestMessage->width());
        ui->latestMessage->setText(elidedText);
    });

    float svgWidgetAspectRatio = 74.46 / 122.88;
    int svgWidgetSize = 14;

    svgStateWidget = new QSvgWidget(":/icons/message/state-" + state + ".svg", this);
    svgStateWidget->setFixedSize(svgWidgetSize, svgWidgetAspectRatio * svgWidgetSize);
    svgStateWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    ui->firstLine->insertWidget(3, svgStateWidget);
}

void chatCard::setUnreadAmount(const QString& unreadAmount) {
    if (unreadAmount.isEmpty()) {
        ui->unreadAmount->hide();
    } else {
        ui->unreadAmount->show();
        ui->unreadAmount->setText(unreadAmount);
    }
}

void chatCard::setIsOnline(const bool& isOnline) {
    if (isOnline) {
        ui->onlineIndicator->show();
    } else {
        ui->onlineIndicator->hide();
    }
}

void chatCard::setIcon(const QString& svgPath) {
    QSvgWidget *svgWidget = new QSvgWidget(svgPath.isEmpty() ? ":/icons/profile/grinning_face_with_big_eyes.svg" : ":/icons/profile/" + svgPath + ".svg", this);
    svgWidget->setFixedSize(27, 27);
    svgWidget->setAutoFillBackground(true);
    svgWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    int indicatorX = ui->chatIconContainer->width() - ui->onlineIndicator->width();
    int indicatorY = ui->chatIconContainer->height() - ui->onlineIndicator->height();

    int svgX = (ui->chatIconContainer->width() - svgWidget->width()) / 2;
    int svgY = (ui->chatIconContainer->height() - svgWidget->height()) / 2;

    ui->chatCardRootLayout->insertWidget(0, svgWidget);

    svgWidget->setParent(ui->chatIconContainer);
    svgWidget->move(svgX, svgY);

    ui->onlineIndicator->setParent(ui->chatIconContainer);
    ui->onlineIndicator->move(indicatorX, indicatorY);
}
