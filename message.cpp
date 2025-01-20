#include "message.h"
#include "QSvgWidget.h"
#include "qtextedit.h"
#include "qtimer.h"
#include "ui_message.h"

message::message(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::message)
{
    ui->setupUi(this);
}

message::~message()
{
    if (svgStateWidget != nullptr) {
        delete svgStateWidget;
    }
    delete ui;
}

void highlightTextAndSetLabel(const QString& messageText, const QString& filterSubstring, QLabel* label) {
    QString highlightedText = messageText;
    int index = highlightedText.indexOf(filterSubstring, 0);
    while (index >= 0) {
        highlightedText.insert(index, "<span style='background-color: orange;'>");
        highlightedText.insert(index + filterSubstring.length() + 36, "</span>");
        index = highlightedText.indexOf(filterSubstring, index + filterSubstring.length() + 36);
    }
    label->setTextFormat(Qt::RichText);
    label->setText(highlightedText);
}

void message::setData(const QString& messageText, const QString& messageTime, const QString& filterSubstring, const bool& isFromCompanion) {
    if (messageTime.isEmpty()) {
        ui->messageText->setText(messageText);
        ui->messageDetailsLayout->deleteLater();
        ui->messageTime->hide();
    }
    else {

        QString messageTextWithInvisibleSpaces;
        if (messageTime == "55:55") {
            messageTextWithInvisibleSpaces = messageText;
            ui->messageTime->hide();
            ui->messageDetailsLayout->deleteLater();
        } else {
            ui->messageTime->setText(messageTime);

            for (int i = 0; i < messageText.length(); ++i) {
                messageTextWithInvisibleSpaces.append(messageText[i]);
                messageTextWithInvisibleSpaces.append(QChar(0x200b));
            }
        }

        if (filterSubstring.isEmpty()) {
            ui->messageText->setText(messageTextWithInvisibleSpaces);
        } else {
            QString filterSubstringWithInvisibleSpaces;
            for (int i = 0; i < filterSubstring.length(); ++i) {
                filterSubstringWithInvisibleSpaces.append(filterSubstring[i]);
                filterSubstringWithInvisibleSpaces.append(QChar(0x200b));
            }
            highlightTextAndSetLabel(messageTextWithInvisibleSpaces, filterSubstringWithInvisibleSpaces, ui->messageText);
        }

        if (isFromCompanion) {
            layout()->removeItem(ui->leftSpacer);
        } else {
            layout()->removeItem(ui->rightSpacer);
        }

    }
    QTextEdit *textEdit = new QTextEdit(this);
    textEdit->setPlainText(messageText);
    textEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    textEdit->setFont(QFont("Inter", 10));

    QFontMetrics fm(textEdit->font());
    QStringList lines = textEdit->toPlainText().split("\n");
    int maxWidth = 0;
    foreach(const QString &line, lines) {
        int lineWidth = fm.horizontalAdvance(line);
        if (lineWidth > maxWidth) {
            maxWidth = lineWidth;
        }
    }
    int textWidth = maxWidth > 260 ? 260 : maxWidth + 1 ;

    ui->messageText->setMinimumWidth(textWidth);
    ui->messageText->setMaximumWidth(textWidth);

    ui->messageLayout->addWidget(textEdit);
    textEdit->hide();
}

void message::setState(const QString& messageState) {
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

void message::adjustSize() {
    QTimer::singleShot(0, this, [=]() {
        ui->messageBox->setMinimumHeight(ui->messageBox->height());
        ui->messageBox->setMaximumHeight(ui->messageBox->height());
    });
}
