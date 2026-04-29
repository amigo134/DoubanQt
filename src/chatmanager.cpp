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

int ChatManager::serverUserId() const
{
    return m_serverUserId;
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

// --- Review requests ---

void ChatManager::requestSaveReview(const QString& doubanId, const QString& movieName,
                                     double rating, const QString& content, bool isWished, bool isWatched,
                                     const QString& posterUrl)
{
    QJsonObject obj;
    obj["type"] = "save_review";
    obj["douban_id"] = doubanId;
    obj["movie_name"] = movieName;
    obj["rating"] = rating;
    obj["content"] = content;
    obj["is_wished"] = isWished;
    obj["is_watched"] = isWatched;
    obj["poster_url"] = posterUrl;
    sendJson(obj);
}

void ChatManager::requestGetReview(const QString& doubanId)
{
    QJsonObject obj;
    obj["type"] = "get_review";
    obj["douban_id"] = doubanId;
    sendJson(obj);
}

void ChatManager::requestGetAllReviews()
{
    QJsonObject obj;
    obj["type"] = "get_all_reviews";
    sendJson(obj);
}

void ChatManager::requestDeleteReview(const QString& doubanId)
{
    QJsonObject obj;
    obj["type"] = "delete_review";
    obj["douban_id"] = doubanId;
    sendJson(obj);
}

void ChatManager::requestGetWishList()
{
    QJsonObject obj;
    obj["type"] = "get_wish_list";
    sendJson(obj);
}

void ChatManager::requestGetWatchedList()
{
    QJsonObject obj;
    obj["type"] = "get_watched_list";
    sendJson(obj);
}

void ChatManager::requestGetProfile()
{
    QJsonObject obj;
    obj["type"] = "get_profile";
    sendJson(obj);
}

void ChatManager::requestSaveProfile(const QString& name, const QString& bio)
{
    QJsonObject obj;
    obj["type"] = "save_profile";
    obj["name"] = name;
    obj["bio"] = bio;
    sendJson(obj);
}

void ChatManager::requestSaveAvatar(const QString& avatarPath)
{
    QJsonObject obj;
    obj["type"] = "save_avatar";
    obj["avatar_path"] = avatarPath;
    sendJson(obj);
}

void ChatManager::requestMovieReviews(const QString& doubanId)
{
    QJsonObject obj;
    obj["type"] = "get_movie_reviews";
    obj["douban_id"] = doubanId;
    sendJson(obj);
}

void ChatManager::requestUserReviews(const QString& username)
{
    QJsonObject obj;
    obj["type"] = "get_user_reviews";
    obj["username"] = username;
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
            m_serverUserId = obj["user_id"].toInt();
        }
        emit loginResult(obj["success"].toBool());
    } else if (type == "friend_request") {
        emit friendRequestReceived(obj["from"].toString(),
                                   obj["from_id"].toInt());
    } else if (type == "add_friend_result") {
        emit addFriendResult(obj["success"].toBool(), obj["message"].toString());
    } else if (type == "friend_accepted") {
        emit friendAccepted(obj["username"].toString(),
                            obj["user_id"].toInt());
    } else if (type == "friend_list") {
        QList<FriendInfo> friends;
        QJsonArray arr = obj["friends"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject fo = v.toObject();
            FriendInfo fi;
            fi.userId = fo["user_id"].toInt();
            fi.username = fo["username"].toString();
            fi.online = fo["online"].toBool();
            friends.append(fi);
        }
        emit friendListReceived(friends);
    } else if (type == "recv_msg") {
        emit messageReceived(obj["from"].toString(),
                             obj["content"].toString(),
                             obj["time"].toString(),
                             obj["id"].toInt(),
                             obj["from_id"].toInt());
    } else if (type == "send_msg_result") {
        emit messageSent(obj["to"].toString(),
                         obj["content"].toString(),
                         obj["time"].toString(),
                         obj["id"].toInt(),
                         obj["to_id"].toInt());
    } else if (type == "offline_msg") {
        QJsonArray arr = obj["messages"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject mo = v.toObject();
            emit messageReceived(mo["from"].toString(),
                                 mo["content"].toString(),
                                 mo["time"].toString(),
                                 mo["id"].toInt(),
                                 mo["from_id"].toInt());
        }
    } else if (type == "online_status") {
        emit onlineStatusChanged(obj["username"].toString(),
                                 obj["online"].toBool(),
                                 obj["user_id"].toInt());
    } else if (type == "chat_history") {
        QString with = obj["with"].toString();
        bool hasMore = obj["has_more"].toBool();
        QList<ChatMsg> messages;
        QJsonArray arr = obj["messages"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject mo = v.toObject();
            ChatMsg msg;
            msg.id = mo["id"].toInt();
            msg.fromId = mo["from_id"].toInt();
            msg.from = mo["from"].toString();
            msg.content = mo["content"].toString();
            msg.time = mo["time"].toString();
            msg.isOwn = mo["is_own"].toBool();
            msg.to = msg.isOwn ? with : m_username;
            messages.append(msg);
        }
        emit chatHistoryReceived(with, messages, hasMore);
    } else if (type == "review_result") {
        bool success = obj["success"].toBool();
        QString doubanId = obj["douban_id"].toString();
        if (success && obj.contains("review")) {
            QJsonObject r = obj["review"].toObject();
            UserReview review;
            review.id = r["id"].toInt();
            review.userId = r["user_id"].toInt();
            review.doubanId = r["douban_id"].toString();
            review.movieName = r["movie_name"].toString();
            review.rating = r["rating"].toDouble();
            review.content = r["content"].toString();
            review.isWished = r["is_wished"].toBool();
            review.isWatched = r["is_watched"].toBool();
            review.posterUrl = r["poster_url"].toString();
            review.createTime = r["create_time"].toString();
            emit reviewReceived(review);
        }
        emit reviewSaved(success, doubanId);
    } else if (type == "reviews_list") {
        QList<UserReview> list;
        QJsonArray arr = obj["reviews"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject r = v.toObject();
            UserReview review;
            review.id = r["id"].toInt();
            review.userId = r["user_id"].toInt();
            review.doubanId = r["douban_id"].toString();
            review.movieName = r["movie_name"].toString();
            review.rating = r["rating"].toDouble();
            review.content = r["content"].toString();
            review.isWished = r["is_wished"].toBool();
            review.isWatched = r["is_watched"].toBool();
            review.posterUrl = r["poster_url"].toString();
            review.createTime = r["create_time"].toString();
            list.append(review);
        }
        emit reviewsListReceived(list);
    } else if (type == "delete_review_result") {
        emit reviewDeleted(obj["success"].toBool(), obj["douban_id"].toString());
    } else if (type == "wish_list") {
        QList<UserReview> list;
        QJsonArray arr = obj["movies"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject r = v.toObject();
            UserReview review;
            review.doubanId = r["douban_id"].toString();
            review.movieName = r["movie_name"].toString();
            review.rating = r["rating"].toDouble();
            review.posterUrl = r["poster_url"].toString();
            review.isWished = true;
            list.append(review);
        }
        emit wishListReceived(list);
    } else if (type == "watched_list") {
        QList<UserReview> list;
        QJsonArray arr = obj["movies"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject r = v.toObject();
            UserReview review;
            review.doubanId = r["douban_id"].toString();
            review.movieName = r["movie_name"].toString();
            review.rating = r["rating"].toDouble();
            review.posterUrl = r["poster_url"].toString();
            review.isWatched = true;
            list.append(review);
        }
        emit watchedListReceived(list);
    } else if (type == "profile_data") {
        emit profileReceived(obj["name"].toString(),
                             obj["bio"].toString(),
                             obj["avatar_path"].toString());
    } else if (type == "profile_saved") {
        emit profileSaved(obj["success"].toBool());
    } else if (type == "avatar_saved") {
        emit avatarSaved(obj["success"].toBool());
    } else if (type == "movie_reviews") {
        QString doubanId = obj["douban_id"].toString();
        QList<UserReview> list;
        QJsonArray arr = obj["reviews"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject r = v.toObject();
            UserReview review;
            review.id = r["id"].toInt();
            review.userId = r["user_id"].toInt();
            review.doubanId = doubanId;
            review.username = r["username"].toString();
            review.movieName = r["movie_name"].toString();
            review.rating = r["rating"].toDouble();
            review.content = r["content"].toString();
            review.createTime = r["create_time"].toString();
            list.append(review);
        }
        emit movieReviewsReceived(doubanId, list);
    } else if (type == "user_reviews") {
        QString username = obj["username"].toString();
        QList<UserReview> list;
        QJsonArray arr = obj["reviews"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject r = v.toObject();
            UserReview review;
            review.id = r["id"].toInt();
            review.userId = r["user_id"].toInt();
            review.doubanId = r["douban_id"].toString();
            review.movieName = r["movie_name"].toString();
            review.rating = r["rating"].toDouble();
            review.content = r["content"].toString();
            review.isWished = r["is_wished"].toBool();
            review.isWatched = r["is_watched"].toBool();
            review.posterUrl = r["poster_url"].toString();
            review.createTime = r["create_time"].toString();
            list.append(review);
        }
        emit userReviewsReceived(username, list);
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
