#include "mainwindow.h"

#include <QApplication>

#include <QFile>
#include <QString>
#include <QTextStream>
#include <QStyleFactory>
#include <QXmlStreamReader>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    QPalette palette = qApp->palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::WindowText, Qt::black);
    qApp->setPalette(palette);

    QLocale::setDefault(QLocale(QLocale::Russian));

    QFile styleFile(":/styles/core.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        QString styleSheet = stream.readAll();
        styleFile.close();

        application.setStyleSheet(styleSheet);
    } else {
        qDebug() << "Failed to open styles file.";
    }

    MainWindow mainWindow;
    mainWindow.show();
    return application.exec();
}
