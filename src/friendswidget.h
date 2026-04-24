#pragma once
#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>
#include "chatmodel.h"
#include "chatmanager.h"

class FriendsWidget : public QWidget {
    Q_OBJECT
public:
    explicit FriendsWidget(ChatManager* chatMgr, QWidget* parent = nullptr);

    void refreshFriendList();

signals:
    void connectionStatusChanged(bool connected);

private slots:
    void onFriendItemClicked(QListWidgetItem* item);
    void onSendClicked();
    void onAddFriendClicked();
    void onFriendRequestClicked();
    void onLoginResult(bool success);
    void onFriendListReceived(const QList<FriendInfo>& friends);
    void onFriendRequestReceived(const QString& from);
    void onAddFriendResult(bool success, const QString& message);
    void onFriendAccepted(const QString& username);
    void onMessageReceived(const QString& from, const QString& content, const QString& time);
    void onOnlineStatusChanged(const QString& username, bool online);
    void onChatDisconnected();

private:
    void buildUI();
    void updateFriendListUI();
    void openChatWith(const QString& username);
    void appendChatMessage(const QString& from, const QString& content, const QString& time, bool isOwn);
    void showPlaceholder();
    void updateRequestBadge();

    ChatManager* m_chatMgr;

    QListWidget* m_friendList;
    QPushButton* m_addFriendBtn;
    QPushButton* m_friendReqBtn;
    QLabel* m_statusLabel;

    QWidget* m_chatArea;
    QLabel* m_chatTitle;
    QTextEdit* m_chatHistory;
    QLineEdit* m_msgInput;
    QPushButton* m_sendBtn;
    QWidget* m_placeholder;

    QString m_currentChatUser;
    QList<FriendInfo> m_friends;
    QStringList m_pendingRequests;
    QMap<QString, QList<ChatMsg>> m_chatMessages;
};
