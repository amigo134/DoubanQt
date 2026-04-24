#pragma once
#include <QObject>
#include <QWebSocket>
#include "chatmodel.h"

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

signals:
    void connected();
    void connectionFailed(const QString& error);
    void loginResult(bool success);
    void friendRequestReceived(const QString& from);
    void addFriendResult(bool success, const QString& message);
    void friendAccepted(const QString& username);
    void friendListReceived(const QList<FriendInfo>& friends);
    void messageReceived(const QString& from, const QString& content, const QString& time);
    void onlineStatusChanged(const QString& username, bool online);
    void disconnected();

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
