#pragma once
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QMap>
#include "serverdb.h"

class ChatServer : public QObject {
    Q_OBJECT
public:
    explicit ChatServer(quint16 port, QObject* parent = nullptr);

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString& message);
    void onClientDisconnected();

private:
    void handleLogin(QWebSocket* socket, const QJsonObject& obj);
    void handleAddFriend(QWebSocket* socket, const QJsonObject& obj);
    void handleAcceptFriend(QWebSocket* socket, const QJsonObject& obj);
    void handleRejectFriend(QWebSocket* socket, const QJsonObject& obj);
    void handleSendMessage(QWebSocket* socket, const QJsonObject& obj);
    void handleGetFriendList(QWebSocket* socket, const QJsonObject& obj);

    void sendToSocket(QWebSocket* socket, const QJsonObject& obj);
    void sendToUser(int userId, const QJsonObject& obj);
    void notifyOnlineStatus(int userId, bool online);

    QWebSocketServer* m_server;
    ServerDb* m_db;
    QMap<QWebSocket*, int> m_socketToUser;
    QMap<int, QWebSocket*> m_userToSocket;
};
