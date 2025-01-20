#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chatcardstate.h"
#include "openssl/aes.h"
#include "peerconnection.h"
#include <QMainWindow>
#include <QSqlDatabase>
#include <vector>
#include "messagestate.h"
#include "qscrollbar.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void showAccountChoiceDialog();

public slots:
    void switchAccount(const QString& accountId);
    void closeApp();

private slots:
    void on_sendMessageButton_clicked();
    void on_messageInput_textChanged();
    void on_sendFileButton_clicked();

    void on_sendStickerButton_clicked();

    void on_sideBarSearch_textChanged(const QString &arg1);

    void on_detailsButton_clicked();

    void on_settingsButton_clicked();

    void on_faqButton_clicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    bool shouldNotScrollMessagesDown = false;
    Ui::MainWindow *ui;
    QString nickname, encryptedPassword, iconName, accountId, ipAddress, portString;
    quint16 port;
    QSqlDatabase dbConnection;
    AES_KEY* aesKeyFromPassword;
    std::vector<chatCardState*> chatCardStateObjects;
    std::vector<messageState*> messageStateObjects;
    chatCardState *openedChat = nullptr;
    PeerConnection *peerConnection = new class PeerConnection(nullptr, &chatCardStateObjects);
    QScrollBar *messagesListVerticalScrollBar;
    void updateReadStatusForMessages();
    void sendTextMessage();
    void resetMessageInput();
    void loadUserAccount(const QString& u_accountId, AES_KEY *u_aesKeyFromPassword);
    void handleChatCardClick(const QString& chatId);
    void handleChatAdded(const QString& connectionAdress_);
    void proccessInitialConnectionRequest(const QJsonDocument& connectionDetails);
    void proccessMessageReceiving(const QJsonDocument& messageDetails);
    void proccessMessageSyncReceiving(const QJsonDocument& messageDetails);
    void proccessMessageStateSent(const QJsonDocument& messageDetails);
    void proccessMessageSyncSent(const QJsonDocument& messageDetails);
    void proccessMessageSent(const QJsonDocument& messageDetails);
    void proccessOnChatConnectedEvent(const QString& chatId);
    void proccessOnChatDisconnectedEvent(const QString& chatId);
    void proccessRequestAccepted(const QString& connectionNickname, const QString& connectionIpAddress, const quint16& connectionPort, const QString& connectionChatId, const QString& rsaPublicKey);
    void proccessRequestRejected(const QString& connectionNickname, const QString& connectionIpAddress, const quint16& connectionPort, const QString& connectionChatId, const QString& rsaPublicKey);
    void proccessRequestBanned(const QString& connectionNickname, const QString& connectionIpAddress, const quint16& connectionPort, const QString& connectionChatId, const QString& rsaPublicKey);
    void addChat(const QString& newChatUuid, const QString& newChatTitle, const QString& newChatAesKey, const QString& newChatIpAddress, const quint16& newChatPort, bool isBanned = false, bool isBannedByCompanion = false, bool isFromCompanion = false);
    void extracted(const QString &chatId, const QString &messageId);
    void addMessage(const QString &chatId, const QString &messageId,
                    const QString &messageType,
                    const QDateTime &messageTimestamp,
                    const QString &messageTimestampString,
                    const QString &messageSender,
                    const QJsonObject &messageValue,
                    const QString &messageState, const bool& shouldRenderMessage = true);
    void renderChatCard(chatCardState* chatCardStateObject_);
    void reRenderChatCard(chatCardState* chatCardStateObject_);
    void renderChatsList(const QString& filterSubstring = "");
    void renderMessage(messageState* messageStateObject_, const QString& filterSubstring = "", const bool& enableAnimation = true);
    void moveMessageRenderQueue(const QString& filterSubstring, const bool& enableAnimation);
    void reRenderMessage(messageState* messageStateObject_, const QString& filterSubstring = "", const bool& enableAnimation = true);
    void renderMessagesList(const QString& filterSubstring = "", const bool& reFetchMessages = false, const bool& movePaginationForwards = false);
    void sendStickerMessage(const QString& stickerName);
    void updateChatIcon(const QString& iconName);
    void updateChatData(const QString& newConnectionAddress, const QString& newNickname);
    void updateChatBan();
    void removeChat();
    void updateAccountIcon(const QString& iconName);
    void updateAccountData(const qint16& newPort, const QString& newNickname);
    void removeAccount();
    void logoutAccount();
    void scrollBarValueChanged(int value);
    void openChat(const QString& chatId);
    void closeOpenedChat(const bool showHelpCenter = false);
    void disableTopBar();
    void enableTopBar();
    void disableBottomBar();
    void enableBottomBar();
};
#endif // MAINWINDOW_H
