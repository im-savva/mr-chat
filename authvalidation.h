#ifndef AUTHVALIDATION_H
#define AUTHVALIDATION_H
#include "QString.h"

std::pair<bool, QString> checkPassword(const QString& password);
std::pair<bool, QString> checkNickname(const QString& nickname);
std::pair<bool, QString> checkPort(const QString& port);
std::pair<bool, QString> checkConnectionAdress(const QString& ip);

#endif // AUTHVALIDATION_H
