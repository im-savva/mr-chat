#include "filemessage.h"
#include "qevent.h"
#include "qtextedit.h"
#include "ui_filemessage.h"
#include <QTimer>

fileMessage::fileMessage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::fileMessage)
{
    ui->setupUi(this);
}

fileMessage::~fileMessage()
{
    if (svgStateWidget != nullptr) {
        delete svgStateWidget;
    }
    delete ui;
}

void fileMessage::mouseReleaseEvent(QMouseEvent *event)
{
    // Crashes the app
    // QWidget::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton) {
        emit fileMessageClicked();
    }
}

void highlightTextAndSetElitedLabel(const QString& messageText, const QString& filterSubstring, QLabel* label, int maxWidth) {
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

QString formatSize(qint64 bytes) {
    static const char* suffixes[] = {"Б", "КБ", "МБ", "ГБ", "ТБ", "ПБ", "ЭБ"};
    int suffixIndex = 0;
    double size = bytes;

    while (size >= 1024 && suffixIndex < 6) {
        size /= 1024;
        suffixIndex++;
    }

    return QString("%1 %2").arg(size, 0, 'f', 1).arg(suffixes[suffixIndex]);
}

void fileMessage::setData(const QString& fileName, const qint64& fileSize, const QString& messageTime, const QString& filterSubstring, const bool& isFromCompanion) {
    ui->messageTime->setText(messageTime);
    ui->fileSize->setText(formatSize(fileSize));

    ui->fileNameBack->hide();
    ui->fileNameFront->setText(fileName);

    QTextEdit *textEdit = new QTextEdit(this);
    textEdit->setPlainText(fileName);
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

    ui->messageTitleLayout->setMinimumWidth(textWidth);
    ui->messageTitleLayout->setMaximumWidth(textWidth);
    ui->messageTitleLayout->setToolTip(fileName);

    ui->messageLayout_->addWidget(textEdit);
    textEdit->hide();

    if (filterSubstring.isEmpty()) {
        if (maxWidth > 260) {
            int lastDotIndex = fileName.lastIndexOf('.');
            if (lastDotIndex != -1) {
                if (lastDotIndex > 15) {
                    lastDotIndex = lastDotIndex - 10;
                }
                QString beforeDot = fileName.left(lastDotIndex).trimmed();
                QString afterDot = fileName.mid(lastDotIndex).trimmed();
                ui->fileNameBack->show();

                ui->fileNameFront->setText(beforeDot);
                ui->fileNameBack->setText(afterDot);

                QTimer::singleShot(3, this, [=]() {
                    QString text(beforeDot);
                    QFontMetrics metrics(ui->fileNameFront->font());
                    QString elidedFileName = metrics.elidedText(text, Qt::ElideRight, ui->fileNameFront->width());
                    ui->fileNameFront->setText(elidedFileName);
                });
            } else {
                ui->fileNameBack->hide();
                ui->fileNameFront->setText(fileName);
                QTimer::singleShot(3, this, [=]() {
                    QString text(fileName);
                    QFontMetrics metrics(ui->fileNameFront->font());
                    QString elidedFileName = metrics.elidedText(text, Qt::ElideRight, ui->fileNameFront->width());
                    ui->fileNameFront->setText(elidedFileName);
                });
            }
        } else {
            ui->fileNameBack->hide();
            ui->fileNameFront->setText(fileName);
        }
    } else {
        highlightTextAndSetElitedLabel(fileName, filterSubstring, ui->fileNameFront, maxWidth);
    }

    if (isFromCompanion) {
        layout()->removeItem(ui->leftSpacer);
    } else {
        layout()->removeItem(ui->rightSpacer);
    }
}

void fileMessage::setState(const QString& messageState) {
    float svgWidgetAspectRatio = 74.46 / 122.88;
    int svgWidgetSize = 14;

    if (svgStateWidget != nullptr) {
        delete svgStateWidget;
    }

    svgStateWidget = new QSvgWidget(":/icons/message/state-" + messageState + ".svg", this);
    svgStateWidget->setFixedSize(svgWidgetSize, svgWidgetAspectRatio * svgWidgetSize);
    svgStateWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    ui->messageDetailsLayout->insertWidget(2, svgStateWidget);
}
