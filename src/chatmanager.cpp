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
                             obj["time"].toString());
    } else if (type == "offline_msg") {
        QJsonArray arr = obj["messages"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject mo = v.toObject();
            emit messageReceived(mo["from"].toString(),
                                 mo["content"].toString(),
                                 mo["time"].toString());
        }
    } else if (type == "online_status") {
        emit onlineStatusChanged(obj["username"].toString(),
                                 obj["online"].toBool());
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
