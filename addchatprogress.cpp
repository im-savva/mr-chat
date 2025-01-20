#include "addchatprogress.h"
#include "peerconnection.h"
#include "qjsondocument.h"
#include "qtimer.h"
#include "ui_addchatprogress.h"
#include "encryption.h"

addchatprogress::addchatprogress(QWidget *parent)
    : QDialog(parent), ui(new Ui::addchatprogress)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);    
    show();
}

void addchatprogress::setContext(const QString& ipAddress_, const quint16& port_, const QString& ipAddress, const quint16& port, const QString& nickname) {

    peerInitialConnection.start(&port);

    QString portString_ = QString::number(port_);

    if (ipAddress != ipAddress_ || port != port_) {
        setContent("Добавление чата", "Подключение к " + ipAddress_ + ":" + portString_, 33);

        peerInitialConnection.connectToPeer(ipAddress_, port_);

        QTimer::singleShot(1000, this, [this, ipAddress_, portString_, ipAddress, &port, nickname]() {

            rsaKeyPair = generateRSAKeyPair();
            QString publicKeyString = rsaPublicKeyToString(rsaKeyPair);
            QDateTime currentDateTime = QDateTime::currentDateTime();
            QDateTime expirationDateTime = currentDateTime.addSecs(60);
            QString expirationString = expirationDateTime.toString(Qt::ISODate);
            QJsonObject jsonObject;
            jsonObject["type"] = "initial-connection-request";
            QJsonObject credentials;
            credentials["chatId"] = chatId;
            credentials["port"] = port;
            credentials["ipAddress"] = ipAddress;
            credentials["nickname"] = nickname;
            credentials["expire"] = expirationString;
            credentials["rsaPublicKey"] = publicKeyString;
            jsonObject["credentials"] = credentials;

            bool isRemoteUserOnline = peerInitialConnection.sendConnectionMessage(jsonObject);

            if (isRemoteUserOnline) {
                setContent(ipAddress_ + ":" + portString_, "Отправка запроса на добавление чата.", 66);
                connect(&peerInitialConnection, &PeerInitialConnection::initialConnectionResponse, this, &addchatprogress::proccessInitialConnectionResponse);
                QTimer::singleShot(60000, this, [this]() {
                    if (showTimeoutMessage) {
                        setContent("Возникла ошибка", "Превышено время ожидания ответа.", 100);
                        QTimer::singleShot(5000, this, [this]() {
                            shutDownAddingChatProgress();
                        });
                    }
                });
            } else {
                setContent("Возникла ошибка", "Похоже, " + ipAddress_ + ":" + portString_ + " не в сети.", 100);
                QTimer::singleShot(5000, this, [this]() {
                    shutDownAddingChatProgress();
                });
            }
        });
    } else {
        setContent("Возникла ошибка", "Вы не можете подключиться к своему аккаунту.", 100);
        QTimer::singleShot(5000, this, [this]() {
            shutDownAddingChatProgress();
        });
    }

}

void addchatprogress::shutDownAddingChatProgress() {
    peerInitialConnection.stop();
    close();
}

void addchatprogress::proccessInitialConnectionResponse(const QJsonDocument& connectionDetails) {
    QString connectionChatId = connectionDetails["credentials"]["chatId"].toString();

    if (chatId == connectionChatId) {
        showTimeoutMessage = false;
        QString response = connectionDetails["data"].toString();
        QString errorMessage;
        if (response == "rejected") {
            setContent("Возникла ошибка", "Запрос отклонен.", 100);
            QTimer::singleShot(5000, this, [this]() {
                shutDownAddingChatProgress();
            });
        }
        else if (response == "banned") {
            QString nickname_ = connectionDetails["credentials"]["nickname"].toString();
            QString ipAddress_ = connectionDetails["credentials"]["ipAddress"].toString();
            quint16 port_ = connectionDetails["credentials"]["port"].toInteger();
            QString aesEncryptedKey = connectionDetails["credentials"]["aesEncryptedKey"].toString();

            QByteArray receivedEncryptedAesKeyBase64 = QByteArray::fromBase64(aesEncryptedKey.toUtf8());
            const unsigned char *receivedEncryptedAesKey = reinterpret_cast<const unsigned char *>(receivedEncryptedAesKeyBase64.constData());
            int receivedEncryptedAesKeyLength = receivedEncryptedAesKeyBase64.size();
            int decryptedAesKeyLength;
            unsigned char *decryptedAesKey = rsaDecryptAESKey(rsaKeyPair, receivedEncryptedAesKey, receivedEncryptedAesKeyLength, &decryptedAesKeyLength);

            if (decryptedAesKey == nullptr) {
                setContent("Возникла ошибка", "Сквозное шифрование не может быть обеспечено.", 100);
                QTimer::singleShot(5000, this, [this]() {
                    shutDownAddingChatProgress();
                });
            } else {
                QString decryptedAesKeyString = aesKeyToString((const AES_KEY *)decryptedAesKey);
                setContent("Вас заблокировали", "Нельзя отправлять сообщения в чат \"" + nickname_ + "\".", 100);
                emit chatAdded(chatId, nickname_, decryptedAesKeyString, ipAddress_, port_, false, true);
                QTimer::singleShot(5000, this, [this]() {
                    shutDownAddingChatProgress();
                });
            }
        }
        else if (response == "accepted") {
            QString nickname_ = connectionDetails["credentials"]["nickname"].toString();
            QString ipAddress_ = connectionDetails["credentials"]["ipAddress"].toString();
            quint16 port_ = connectionDetails["credentials"]["port"].toInteger();
            QString aesEncryptedKey = connectionDetails["credentials"]["aesEncryptedKey"].toString();

            QByteArray receivedEncryptedAesKeyBase64 = QByteArray::fromBase64(aesEncryptedKey.toUtf8());
            const unsigned char *receivedEncryptedAesKey = reinterpret_cast<const unsigned char *>(receivedEncryptedAesKeyBase64.constData());
            int receivedEncryptedAesKeyLength = receivedEncryptedAesKeyBase64.size();
            int decryptedAesKeyLength;
            unsigned char *decryptedAesKey = rsaDecryptAESKey(rsaKeyPair, receivedEncryptedAesKey, receivedEncryptedAesKeyLength, &decryptedAesKeyLength);

            if (decryptedAesKey == nullptr) {
                setContent("Возникла ошибка", "Сквозное шифрование не может быть обеспечено.", 100);
                QTimer::singleShot(5000, this, [this]() {
                    shutDownAddingChatProgress();
                });
            } else {
                QString decryptedAesKeyString = aesKeyToString((const AES_KEY *)decryptedAesKey);
                setContent("Готово!", "Чат \"" + nickname_ + "\" успешно добавлен.", 100);
                emit chatAdded(chatId, nickname_, decryptedAesKeyString, ipAddress_, port_, false);
                QTimer::singleShot(5000, this, [this]() {
                    shutDownAddingChatProgress();
                });
            }
        } else {
            setContent("Возникла ошибка", "Был получен неправильный ответ (00000).", 100);
            QTimer::singleShot(5000, this, [this]() {
                shutDownAddingChatProgress();
            });
        }
    }
}

void addchatprogress::setContent(const QString& title, const QString& description, const qint16& progress) {
    ui->title->setText(title);
    ui->description->setText(description);
    ui->progressBar->setValue(progress);
}

addchatprogress::~addchatprogress()
{
    delete ui;
}
