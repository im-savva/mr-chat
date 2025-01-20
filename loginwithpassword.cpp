#include "loginwithpassword.h"
#include "ui_loginwithpassword.h"
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QCryptographicHash>

std::pair<bool, QString> loginWithPassword::checkPassword(const QString& password) {
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

loginWithPassword::loginWithPassword(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::loginWithPassword)
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

    // connect(ui->welcomeScreenContinue, &QPushButton::clicked, this, &loginWithPassword::onContinueClicked);
}

QByteArray loginWithPassword::encryptPassword(const QString& password) {
    QByteArray passwordData = password.toUtf8();
    QByteArray hashedPassword = QCryptographicHash::hash(passwordData, QCryptographicHash::Sha256);
    return hashedPassword;
}

void loginWithPassword::onContinueClicked() {
    QString password = ui->passwordInput->text();

    QByteArray encryptedPassword = encryptPassword(password);

    emit passwordReady(encryptedPassword);

    hide();
}

loginWithPassword::~loginWithPassword()
{
    delete ui;
}
