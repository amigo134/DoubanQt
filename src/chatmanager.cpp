#include "chatmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QCoreApplication>
#include <QDebug>

ChatManager::ChatManager(QObject* parent)
    : QObject(parent)
    , m_socket(new QWebSocket())
    , m_serverUrl("ws://localhost:8765")
{
    QSettings settings(QCoreApplication::applicationDirPath() + "/config.ini",
                       QSettings::IniFormat);
    QString savedUrl = settings.value("chat/server_url", "").toString();
    if (!savedUrl.isEmpty()) m_serverUrl = savedUrl;

    connect(m_socket, &QWebSocket::connected, this, &ChatManager::onConnected);
    connect(m_socket, &QWebSocket::textMessageReceived,
            this, &ChatManager::onTextMessageReceived);
    connect(m_socket, &QWebSocket::disconnected, this, &ChatManager::onDisconnected);
}

void ChatManager::connectToServer(const QString& username, const QString& passwordHash)
{
    m_username = username;
    m_socket->open(QUrl(m_serverUrl));

    QJsonObject obj;
    obj["type"] = "login";
    obj["username"] = username;
    obj["password"] = passwordHash;

    QMetaObject::Connection* conn = new QMetaObject::Connection();
    *conn = connect(m_socket, &QWebSocket::connected, this, [this, obj, conn]() {
        sendJson(obj);
        disconnect(*conn);
        delete conn;
    });
}

void ChatManager::disconnectFromServer()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->close();
    }
}

bool ChatManager::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

QString ChatManager::currentUsername() const
{
    return m_username;
}

void ChatManager::sendAddFriend(const QString& username)
{
    QJsonObject obj;
    obj["type"] = "add_friend";
    obj["username"] = username;
    sendJson(obj);
}

void ChatManager::acceptFriend(const QString& username)
{
    QJsonObject obj;
    obj["type"] = "accept_friend";
    obj["username"] = username;
    sendJson(obj);
}

void ChatManager::rejectFriend(const QString& username)
{
    QJsonObject obj;
    obj["type"] = "reject_friend";
    obj["username"] = username;
    sendJson(obj);
}

void ChatManager::sendMessage(const QString& to, const QString& content)
{
    QJsonObject obj;
    obj["type"] = "send_msg";
    obj["to"] = to;
    obj["content"] = content;
    sendJson(obj);
}

void ChatManager::requestFriendList()
{
    QJsonObject obj;
    obj["type"] = "get_friend_list";
    sendJson(obj);
}

void ChatManager::requestChatHistory(const QString& with, int limit, int beforeMsgId)
{
    QJsonObject obj;
    obj["type"] = "get_chat_history";
    obj["with"] = with;
    obj["limit"] = limit;
    obj["before_msg_id"] = beforeMsgId;
    sendJson(obj);
}

void ChatManager::onConnected()
{
    qDebug() << "ChatManager: connected to server";
    emit connected();
}

void ChatManager::onTextMessageReceived(const QString& message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "login_result") {
        if (obj["success"].toBool()) {
            m_username = obj["username"].toString();
        }
        emit loginResult(obj["success"].toBool());
        if (obj["success"].toBool()) {
            requestFriendList();
        }
    } else if (type == "friend_request") {
        emit friendRequestReceived(obj["from"].toString());
    } else if (type == "add_friend_result") {
        emit addFriendResult(obj["success"].toBool(), obj["message"].toString());
    } else if (type == "friend_accepted") {
        emit friendAccepted(obj["username"].toString());
    } else if (type == "friend_list") {
        QList<FriendInfo> friends;
        QJsonArray arr = obj["friends"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject fo = v.toObject();
            FriendInfo fi;
            fi.username = fo["username"].toString();
            fi.online = fo["online"].toBool();
            friends.append(fi);
        }
        emit friendListReceived(friends);
    } else if (type == "recv_msg") {
        emit messageReceived(obj["from"].toString(),
                             obj["content"].toString(),
                             obj["time"].toString(),
                             obj["id"].toInt());
    } else if (type == "send_msg_result") {
        emit messageSent(obj["to"].toString(),
                         obj["content"].toString(),
                         obj["time"].toString(),
                         obj["id"].toInt());
    } else if (type == "offline_msg") {
        QJsonArray arr = obj["messages"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject mo = v.toObject();
            emit messageReceived(mo["from"].toString(),
                                 mo["content"].toString(),
                                 mo["time"].toString(),
                                 mo["id"].toInt());
        }
    } else if (type == "online_status") {
        emit onlineStatusChanged(obj["username"].toString(),
                                 obj["online"].toBool());
    } else if (type == "chat_history") {
        QString with = obj["with"].toString();
        bool hasMore = obj["has_more"].toBool();
        QList<ChatMsg> messages;
        QJsonArray arr = obj["messages"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject mo = v.toObject();
            ChatMsg msg;
            msg.id = mo["id"].toInt();
            msg.from = mo["from"].toString();
            msg.content = mo["content"].toString();
            msg.time = mo["time"].toString();
            msg.isOwn = mo["is_own"].toBool();
            msg.to = msg.isOwn ? with : m_username;
            messages.append(msg);
        }
        emit chatHistoryReceived(with, messages, hasMore);
    }
}

void ChatManager::onDisconnected()
{
    qDebug() << "ChatManager: disconnected from server";
    emit disconnected();
}

void ChatManager::sendJson(const QJsonObject& obj)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->sendTextMessage(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}
