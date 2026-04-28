#pragma once
#include <QObject>
#include <QWebSocket>
#include "chatmodel.h"
#include "moviemodel.h"

class ChatManager : public QObject {
    Q_OBJECT
public:
    explicit ChatManager(QObject* parent = nullptr);

    void connectToServer(const QString& username, const QString& passwordHash);
    void disconnectFromServer();
    bool isConnected() const;
    QString currentUsername() const;

    void sendAddFriend(const QString& username);
    void acceptFriend(const QString& username);
    void rejectFriend(const QString& username);
    void sendMessage(const QString& to, const QString& content);
    void requestFriendList();
    void requestChatHistory(const QString& with, int limit = 30, int beforeMsgId = 0);

    void requestSaveReview(const QString& doubanId, const QString& movieName,
                           double rating, const QString& content, bool isWished, bool isWatched,
                           const QString& posterUrl);
    void requestGetReview(const QString& doubanId);
    void requestGetAllReviews();
    void requestDeleteReview(const QString& doubanId);
    void requestGetWishList();
    void requestGetWatchedList();
    void requestGetProfile();
    void requestSaveProfile(const QString& name, const QString& bio);
    void requestSaveAvatar(const QString& avatarPath);

signals:
    void connected();
    void connectionFailed(const QString& error);
    void loginResult(bool success);
    void friendRequestReceived(const QString& from);
    void addFriendResult(bool success, const QString& message);
    void friendAccepted(const QString& username);
    void friendListReceived(const QList<FriendInfo>& friends);
    void messageReceived(const QString& from, const QString& content, const QString& time, int msgId = 0);
    void messageSent(const QString& to, const QString& content, const QString& time, int msgId);
    void chatHistoryReceived(const QString& with, const QList<ChatMsg>& messages, bool hasMore);
    void onlineStatusChanged(const QString& username, bool online);
    void disconnected();

    // Review signals
    void reviewSaved(bool success, const QString& doubanId);
    void reviewReceived(const UserReview& review);
    void reviewsListReceived(const QList<UserReview>& reviews);
    void reviewDeleted(bool success, const QString& doubanId);
    void wishListReceived(const QList<UserReview>& list);
    void watchedListReceived(const QList<UserReview>& list);
    // Profile signals
    void profileReceived(const QString& name, const QString& bio, const QString& avatarPath);
    void profileSaved(bool success);
    void avatarSaved(bool success);

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& message);
    void onDisconnected();

private:
    void sendJson(const QJsonObject& obj);

    QWebSocket* m_socket;
    QString m_username;
    QString m_serverUrl;
};
