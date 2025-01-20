#include "initialization_with_password.h"
#include "ui_initialization_with_password.h"
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QCryptographicHash>

std::pair<bool, QString> initialization_with_password::checkPassword(const QString& password) {
    QString errorMessage;

    // Regular expressions for password checks
    QRegularExpression digitRegex("\\d");
    QRegularExpression uppercaseRegex("[A-Z]");
    QRegularExpression lowercaseRegex("[a-z]");
    QRegularExpression validCharsRegex("[^a-zA-Z0-9]");

    // Check if password contains only valid characters: alphanumeric
    if (validCharsRegex.match(password).hasMatch()) {
        errorMessage = "Пожалуйста, используйте только английские буквы и цифры.";
        return std::make_pair(false, errorMessage);
    }

    // Check if password contains at least one digit
    if (!password.contains(digitRegex)) {
        errorMessage = "Пожалуйста, используйте хотя бы одну цифру.";
        return std::make_pair(false, errorMessage);
    }

    // Check if password contains at least one uppercase letter
    if (!password.contains(uppercaseRegex)) {
        errorMessage = "Пожалуйста, используйте хотя бы одну заглавную букву.";
        return std::make_pair(false, errorMessage);
    }

    // Check if password contains at least one lowercase letter
    if (!password.contains(lowercaseRegex)) {
        errorMessage = "Пожалуйста, используйте хотя бы одну прописную букву.";
        return std::make_pair(false, errorMessage);
    }

    if(password.length() < 8){
        errorMessage = "Пожалуйста, используйте от 8 до 16 символов.";
        return std::make_pair(false, errorMessage);
    }

    // Password meets all criteria
    return std::make_pair(true, "");
}

initialization_with_password::initialization_with_password(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::initialization_with_password)
{
    ui->setupUi(this);

    connect(ui->passwordInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto result = checkPassword(text);
        if (result.first) {
            ui->passwordInputStatus->hide();
            ui->welcomeScreenContinue->setEnabled(true);
        } else {
            ui->passwordInputStatus->show();
            ui->passwordInputStatus->setText(result.second);
            ui->welcomeScreenContinue->setEnabled(false);
        }
    });

    connect(ui->welcomeScreenContinue, &QPushButton::clicked, this, &initialization_with_password::onContinueClicked);
}

QByteArray initialization_with_password::encryptPassword(const QString& password) {
    QByteArray passwordData = password.toUtf8();
    QByteArray hashedPassword = QCryptographicHash::hash(passwordData, QCryptographicHash::Sha256);
    return hashedPassword;
}

void initialization_with_password::onContinueClicked() {
    QString password = ui->passwordInput->text();

    QByteArray encryptedPassword = encryptPassword(password);

    emit passwordReady(encryptedPassword);

    hide();
}

initialization_with_password::~initialization_with_password()
{
    delete ui;
}
