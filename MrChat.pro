QT += core gui sql svgwidgets network

LIBS += -lssl -lcrypto

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    accountchoice.cpp \
    accountsettings.cpp \
    addaccount.cpp \
    addchat.cpp \
    addchatprogress.cpp \
    addchatrequest.cpp \
    authvalidation.cpp \
    chatcard.cpp \
    chatcardstate.cpp \
    chatsettings.cpp \
    emojispicker.cpp \
    encryption.cpp \
    filemessage.cpp \
    loginaccount.cpp \
    main.cpp \
    mainwindow.cpp \
    message.cpp \
    messagestate.cpp \
    peerconnection.cpp \
    peerinitialconnection.cpp \
    removeaccountapproval.cpp \
    removechatapproval.cpp \
    stickermessage.cpp \
    useraccount.cpp

HEADERS += \
    accountchoice.h \
    accountsettings.h \
    addaccount.h \
    addchat.h \
    addchatprogress.h \
    addchatrequest.h \
    authvalidation.h \
    chatcard.h \
    chatcardstate.h \
    chatsettings.h \
    emojispicker.h \
    encryption.h \
    filemessage.h \
    loginaccount.h \
    mainwindow.h \
    message.h \
    messagestate.h \
    peerconnection.h \
    peerinitialconnection.h \
    removeaccountapproval.h \
    removechatapproval.h \
    stickermessage.h \
    useraccount.h

FORMS += \
    accountchoice.ui \
    accountsettings.ui \
    addaccount.ui \
    addchat.ui \
    addchatprogress.ui \
    addchatrequest.ui \
    chatcard.ui \
    chatsettings.ui \
    emojispicker.ui \
    filemessage.ui \
    loginaccount.ui \
    mainwindow.ui \
    message.ui \
    removeaccountapproval.ui \
    removechatapproval.ui \
    stickermessage.ui \
    useraccount.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    mrChatDB.db

RESOURCES += \
    faq.qrc \
    icons.qrc \
    styles.qrc



LIBS += -L$$PWD/'../../../../Program Files/OpenSSL-Win64/lib/VC/x64/MT/' -llibssl

INCLUDEPATH += $$PWD/'../../../../Program Files/OpenSSL-Win64/include'
DEPENDPATH += $$PWD/'../../../../Program Files/OpenSSL-Win64/include'
