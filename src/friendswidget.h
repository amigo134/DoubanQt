#pragma once
#include <QWidget>
#include <QListWidget>
#include <QListView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QSet>
#include "chatmodel.h"
#include "chatmessagedelegate.h"
#include "chatmanager.h"
#include "serverapiclient.h"
#include "loadingwidget.h"

class FriendsWidget : public QWidget {
    Q_OBJECT
public:
    explicit FriendsWidget(ChatManager* chatMgr, ServerApiClient* serverApi, QWidget* parent = nullptr);

    void refreshFriendList();

signals:
    void connectionStatusChanged(bool connected);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onFriendItemClicked(QListWidgetItem* item);
    void onSendClicked();
    void onAddFriendClicked();
    void onFriendRequestClicked();
    void onLoginResult(bool success);
    void onFriendListReceived(const QList<FriendInfo>& friends);
    void onFriendRequestReceived(const QString& from, int fromId = 0);
    void onAddFriendResult(bool success, const QString& message);
    void onFriendAccepted(const QString& username, int userId = 0);
    void onMessageReceived(const QString& from, const QString& content, const QString& time, int msgId = 0, int fromId = 0);
    void onMessageSent(const QString& to, const QString& content, const QString& time, int msgId, int toId = 0);
    void onOnlineStatusChanged(const QString& username, bool online, int userId = 0);
    void onChatDisconnected();
    void onChatHistoryReceived(const QString& with, const QList<ChatMsg>& messages, bool hasMore);
    void onChatScrollBarValueChanged(int value);

private:
    void buildUI();
    void updateFriendListUI();
    void openChatWith(const QString& username);
    void showPlaceholder();
    void updateRequestBadge();
    void scrollToBottom();
    void requestHistory();
    void showTopLoadingIndicator();
    void removeTopLoadingIndicator();

    ChatManager* m_chatMgr;
    ServerApiClient* m_serverApi;

    QListWidget* m_friendList;
    QPushButton* m_addFriendBtn;
    QPushButton* m_friendReqBtn;
    QLabel* m_statusLabel;

    QWidget* m_chatArea;
    QLabel* m_chatTitle;
    QListView* m_chatListView;
    ChatMessageModel* m_chatModel;
    ChatMessageDelegate* m_chatDelegate;
    QLineEdit* m_msgInput;
    QPushButton* m_sendBtn;
    QWidget* m_placeholder;
    LoadingWidget* m_loadingIndicator;

    QString m_currentChatUser;
    int m_currentChatUserId = 0;
    QList<FriendInfo> m_friends;
    QStringList m_pendingRequests;

    bool m_isLoadingHistory = false;
    bool m_hasMoreHistory = true;
    bool m_isInitialLoad = false;
    QMap<QString, bool> m_hasMoreHistoryMap;
    QMap<QString, ChatMessageModel*> m_chatModels;
    QSet<QString> m_historyLoadedUsers;
};
