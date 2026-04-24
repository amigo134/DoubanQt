#include "chatserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

ChatServer::ChatServer(quint16 port, QObject* parent)
    : QObject(parent)
    , m_server(new QWebSocketServer("ChatServer", QWebSocketServer::NonSecureMode, this))
    , m_db(new ServerDb(this))
{
    if (!m_db->initialize()) {
        qWarning() << "Server DB init failed";
    }

    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "Chat server listening on port" << port;
    } else {
        qWarning() << "Failed to listen on port" << port;
    }

    connect(m_server, &QWebSocketServer::newConnection,
            this, &ChatServer::onNewConnection);
}

void ChatServer::onNewConnection()
{
    QWebSocket* socket = m_server->nextPendingConnection();
    connect(socket, &QWebSocket::textMessageReceived,
            this, &ChatServer::onTextMessageReceived);
    connect(socket, &QWebSocket::disconnected,
            this, &ChatServer::onClientDisconnected);
    qDebug() << "Client connected:" << socket->peerAddress().toString();
}

void ChatServer::onTextMessageReceived(const QString& message)
{
    auto* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "login") handleLogin(socket, obj);
    else if (type == "add_friend") handleAddFriend(socket, obj);
    else if (type == "accept_friend") handleAcceptFriend(socket, obj);
    else if (type == "reject_friend") handleRejectFriend(socket, obj);
    else if (type == "send_msg") handleSendMessage(socket, obj);
    else if (type == "get_friend_list") handleGetFriendList(socket, obj);
}

void ChatServer::onClientDisconnected()
{
    auto* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    int userId = m_socketToUser.value(socket, 0);
    if (userId > 0) {
        m_userToSocket.remove(userId);
        m_socketToUser.remove(socket);
        notifyOnlineStatus(userId, false);
        qDebug() << "User disconnected:" << userId;
    }
    socket->deleteLater();
}

void ChatServer::handleLogin(QWebSocket* socket, const QJsonObject& obj)
{
    QString username = obj["username"].toString();
    QString passwordHash = obj["password"].toString();

    int userId = m_db->loginUser(username, passwordHash);
    if (userId == 0) {
        userId = m_db->registerUser(username, passwordHash);
    }
    if (userId == 0) {
        QJsonObject resp;
        resp["type"] = "login_result";
        resp["success"] = false;
        resp["message"] = "login failed";
        sendToSocket(socket, resp);
        return;
    }

    if (m_userToSocket.contains(userId)) {
        QWebSocket* old = m_userToSocket[userId];
        m_socketToUser.remove(old);
        old->close();
    }

    m_socketToUser[socket] = userId;
    m_userToSocket[userId] = socket;

    QJsonObject resp;
    resp["type"] = "login_result";
    resp["success"] = true;
    resp["user_id"] = userId;
    resp["username"] = username;
    sendToSocket(socket, resp);

    QList<ServerMsg> offlineMsgs = m_db->getOfflineMessages(userId);
    if (!offlineMsgs.isEmpty()) {
        QJsonArray arr;
        for (const auto& m : offlineMsgs) {
            QJsonObject mo;
            mo["from"] = m_db->getUsername(m.fromId);
            mo["content"] = m.content;
            mo["time"] = m.time;
            arr.append(mo);
        }
        QJsonObject offMsg;
        offMsg["type"] = "offline_msg";
        offMsg["messages"] = arr;
        sendToSocket(socket, offMsg);
        m_db->markDelivered(userId);
    }

    notifyOnlineStatus(userId, true);

    QList<int> pendingFrom = m_db->getPendingFromIds(userId);
    for (int fromId : pendingFrom) {
        QJsonObject req;
        req["type"] = "friend_request";
        req["from"] = m_db->getUsername(fromId);
        sendToSocket(socket, req);
    }

    qDebug() << "User logged in:" << username << "id:" << userId;
}

void ChatServer::handleAddFriend(QWebSocket* socket, const QJsonObject& obj)
{
    int fromId = m_socketToUser.value(socket, 0);
    if (fromId == 0) return;

    QString targetName = obj["username"].toString();
    int toId = m_db->getUserId(targetName);

    if (toId == 0) {
        QJsonObject resp;
        resp["type"] = "add_friend_result";
        resp["success"] = false;
        resp["message"] = "user not found";
        sendToSocket(socket, resp);
        return;
    }

    int status = m_db->getFriendshipStatus(fromId, toId);
    if (status == 2) {
        QJsonObject resp;
        resp["type"] = "add_friend_result";
        resp["success"] = false;
        resp["message"] = "already friends";
        sendToSocket(socket, resp);
        return;
    }
    if (status == 1) {
        QJsonObject resp;
        resp["type"] = "add_friend_result";
        resp["success"] = false;
        resp["message"] = "request pending";
        sendToSocket(socket, resp);
        return;
    }

    if (!m_db->addFriendRequest(fromId, toId)) {
        QJsonObject resp;
        resp["type"] = "add_friend_result";
        resp["success"] = false;
        resp["message"] = "request failed";
        sendToSocket(socket, resp);
        return;
    }

    QJsonObject resp;
    resp["type"] = "add_friend_result";
    resp["success"] = true;
    resp["message"] = "request sent";
    sendToSocket(socket, resp);

    if (m_userToSocket.contains(toId)) {
        QJsonObject req;
        req["type"] = "friend_request";
        req["from"] = m_db->getUsername(fromId);
        sendToSocket(m_userToSocket[toId], req);
    }
}

void ChatServer::handleAcceptFriend(QWebSocket* socket, const QJsonObject& obj)
{
    int toId = m_socketToUser.value(socket, 0);
    if (toId == 0) return;

    QString fromName = obj["username"].toString();
    int fromId = m_db->getUserId(fromName);
    if (fromId == 0) return;

    if (m_db->acceptFriend(fromId, toId)) {
        QJsonObject resp;
        resp["type"] = "friend_accepted";
        resp["username"] = fromName;
        sendToSocket(socket, resp);

        if (m_userToSocket.contains(fromId)) {
            QJsonObject notify;
            notify["type"] = "friend_accepted";
            notify["username"] = m_db->getUsername(toId);
            sendToSocket(m_userToSocket[fromId], notify);
        }
    }
}

void ChatServer::handleRejectFriend(QWebSocket* socket, const QJsonObject& obj)
{
    int toId = m_socketToUser.value(socket, 0);
    if (toId == 0) return;

    QString fromName = obj["username"].toString();
    int fromId = m_db->getUserId(fromName);
    if (fromId == 0) return;

    m_db->rejectFriend(fromId, toId);
}

void ChatServer::handleSendMessage(QWebSocket* socket, const QJsonObject& obj)
{
    int fromId = m_socketToUser.value(socket, 0);
    if (fromId == 0) return;

    QString toName = obj["to"].toString();
    QString content = obj["content"].toString();
    QString time = QDateTime::currentDateTime().toString(Qt::ISODate);

    int toId = m_db->getUserId(toName);
    if (toId == 0) {
        QJsonObject resp;
        resp["type"] = "error";
        resp["message"] = "user not found";
        sendToSocket(socket, resp);
        return;
    }

    m_db->saveMessage(fromId, toId, content, time);

    QJsonObject msgToRecipient;
    msgToRecipient["type"] = "recv_msg";
    msgToRecipient["from"] = m_db->getUsername(fromId);
    msgToRecipient["content"] = content;
    msgToRecipient["time"] = time;

    if (m_userToSocket.contains(toId)) {
        sendToSocket(m_userToSocket[toId], msgToRecipient);
        m_db->markDelivered(toId);
    }
}

void ChatServer::handleGetFriendList(QWebSocket* socket, const QJsonObject&)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    QList<int> friendIds = m_db->getFriendIds(userId);
    QJsonArray arr;
    for (int fid : friendIds) {
        QJsonObject fo;
        fo["username"] = m_db->getUsername(fid);
        fo["online"] = m_userToSocket.contains(fid);
        arr.append(fo);
    }

    QJsonObject resp;
    resp["type"] = "friend_list";
    resp["friends"] = arr;
    sendToSocket(socket, resp);
}

void ChatServer::sendToSocket(QWebSocket* socket, const QJsonObject& obj)
{
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->sendTextMessage(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}

void ChatServer::sendToUser(int userId, const QJsonObject& obj)
{
    if (m_userToSocket.contains(userId)) {
        sendToSocket(m_userToSocket[userId], obj);
    }
}

void ChatServer::notifyOnlineStatus(int userId, bool online)
{
    QList<int> friendIds = m_db->getFriendIds(userId);
    QString username = m_db->getUsername(userId);

    QJsonObject notify;
    notify["type"] = "online_status";
    notify["username"] = username;
    notify["online"] = online;

    for (int fid : friendIds) {
        if (m_userToSocket.contains(fid)) {
            sendToSocket(m_userToSocket[fid], notify);
        }
    }
}
