#include "authvalidation.h"
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QCryptographicHash>

std::pair<bool, QString> checkPassword(const QString& password) {
    QString errorMessage;

    QRegularExpression digitRegex("\\d");
    QRegularExpression uppercaseRegex("[A-Z]");
    QRegularExpression lowercaseRegex("[a-z]");
    QRegularExpression validCharsRegex("[^a-zA-Z0-9]");

    if (validCharsRegex.match(password).hasMatch()) {
        errorMessage = "Пожалуйста, используйте только латинские буквы и цифры.";
        return std::make_pair(false, errorMessage);
    }

    if (!password.contains(digitRegex)) {
        errorMessage = "Пожалуйста, используйте хотя бы одну цифру.";
        return std::make_pair(false, errorMessage);
    }

    if (!password.contains(uppercaseRegex)) {
        errorMessage = "Пожалуйста, используйте хотя бы одну заглавную букву.";
        return std::make_pair(false, errorMessage);
    }

    if (!password.contains(lowercaseRegex)) {
        errorMessage = "Пожалуйста, используйте хотя бы одну прописную букву.";
        return std::make_pair(false, errorMessage);
    }

    if(password.length() < 8){
        errorMessage = "Пожалуйста, используйте от 8 до 16 символов.";
        return std::make_pair(false, errorMessage);
    }

    return std::make_pair(true, "Пароль введен корректно.");
}

std::pair<bool, QString> checkNickname(const QString& nickname) {
    QString errorMessage;

    QRegularExpression validCharsRegex("[^a-zA-Z0-9а-яА-Я ]");

    if (validCharsRegex.match(nickname).hasMatch()) {
        errorMessage = "Пожалуйста, используйте только буквы и цифры.";
        return std::make_pair(false, errorMessage);
    }

    if(nickname.length() < 3){
        errorMessage = "Пожалуйста, используйте от 3 до 16 символов.";
        return std::make_pair(false, errorMessage);
    }

    return std::make_pair(true, "Имя пользователя заполнено корректно.");
}

std::pair<bool, QString> checkPort(const QString& port) {
    QString errorMessage;

    QRegularExpression validCharsRegex("[^0-9]");

    if (validCharsRegex.match(port).hasMatch()) {
        errorMessage = "Пожалуйста, используйте только цифры.";
        return std::make_pair(false, errorMessage);
    }

    if(port.length() < 1 || port.length() > 5){
        errorMessage = "Пожалуйста, используйте от 1 до 5 символов.";
        return std::make_pair(false, errorMessage);
    }

    return std::make_pair(true, "Порт для подключения указан корректно.");
}

std::pair<bool, QString> checkConnectionAdress(const QString& ip) {
    QRegularExpression addressPortRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?):(\\d{1,5})$");
    QRegularExpressionMatch match = addressPortRegex.match(ip);
    return std::make_pair(match.hasMatch(), match.hasMatch() ? "Адрес для подключения указан корректно." : "Пожалуйста, введите корректный адрес (например, 123.456.789.0:1234).");
}
