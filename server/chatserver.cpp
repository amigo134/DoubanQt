#include "chatserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <memory>

ChatServer::ChatServer(ServerDb* db, quint16 port, QObject* parent)
    : QObject(parent)
    , m_server(new QWebSocketServer("ChatServer", QWebSocketServer::NonSecureMode, this))
    , m_db(db)
{
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
    else if (type == "get_chat_history") handleGetChatHistory(socket, obj);
    else if (type == "save_review") handleSaveReview(socket, obj);
    else if (type == "get_review") handleGetReview(socket, obj);
    else if (type == "get_all_reviews") handleGetAllReviews(socket, obj);
    else if (type == "delete_review") handleDeleteReview(socket, obj);
    else if (type == "get_wish_list") handleGetWishList(socket, obj);
    else if (type == "get_watched_list") handleGetWatchedList(socket, obj);
    else if (type == "get_profile") handleGetProfile(socket, obj);
    else if (type == "save_profile") handleSaveProfile(socket, obj);
    else if (type == "save_avatar") handleSaveAvatar(socket, obj);
    else if (type == "get_movie_reviews") handleGetMovieReviews(socket, obj);
    else if (type == "get_user_reviews") handleGetUserReviews(socket, obj);
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

// --- Handlers ---

void ChatServer::handleLogin(QWebSocket* socket, const QJsonObject& obj)
{
    QString username = obj["username"].toString();
    QString passwordHash = obj["password"].toString();

    struct LoginData {
        int userId = 0;
        QMap<int, QString> offlineSenderNames; // msgId -> senderName
        QList<ServerMsg> offlineMsgs;
        QMap<int, QString> pendingNames; // fromId -> username
        QList<int> pendingFrom;
        QList<int> friendIds;
        QString myName;
    };
    auto data = std::make_shared<LoginData>();

    runDbAsync([this, username, passwordHash, data]() {
        data->userId = m_db->loginUser(username, passwordHash);
        if (data->userId == 0)
            data->userId = m_db->registerUser(username, passwordHash);
        if (data->userId == 0)
            return;

        data->offlineMsgs = m_db->getOfflineMessages(data->userId);
        for (const auto& m : data->offlineMsgs)
            data->offlineSenderNames[m.id] = m_db->getUsername(m.fromId);

        if (!data->offlineMsgs.isEmpty())
            m_db->markDelivered(data->userId);

        data->pendingFrom = m_db->getPendingFromIds(data->userId);
        for (int fromId : data->pendingFrom)
            data->pendingNames[fromId] = m_db->getUsername(fromId);

        data->friendIds = m_db->getFriendIds(data->userId);
        data->myName = m_db->getUsername(data->userId);
    }, [this, socket, username, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState)
            return;

        if (data->userId == 0) {
            QJsonObject resp;
            resp["type"] = "login_result";
            resp["success"] = false;
            resp["message"] = "login failed";
            sendToSocket(socket, resp);
            return;
        }

        // Replace any existing connection for this user
        if (m_userToSocket.contains(data->userId)) {
            QWebSocket* old = m_userToSocket[data->userId];
            m_socketToUser.remove(old);
            old->close();
        }

        m_socketToUser[socket] = data->userId;
        m_userToSocket[data->userId] = socket;

        QJsonObject resp;
        resp["type"] = "login_result";
        resp["success"] = true;
        resp["user_id"] = data->userId;
        resp["username"] = username;
        sendToSocket(socket, resp);

        // Offline messages
        if (!data->offlineMsgs.isEmpty()) {
            QJsonArray arr;
            for (const auto& m : data->offlineMsgs) {
                QJsonObject mo;
                mo["id"] = m.id;
                mo["from_id"] = m.fromId;
                mo["from"] = data->offlineSenderNames.value(m.id);
                mo["content"] = m.content;
                mo["time"] = m.time;
                arr.append(mo);
            }
            QJsonObject offMsg;
            offMsg["type"] = "offline_msg";
            offMsg["messages"] = arr;
            sendToSocket(socket, offMsg);
        }

        // Notify online status to friends
        {
            QJsonObject notify;
            notify["type"] = "online_status";
            notify["username"] = data->myName;
            notify["online"] = true;
            for (int fid : data->friendIds) {
                if (m_userToSocket.contains(fid))
                    sendToSocket(m_userToSocket[fid], notify);
            }
        }

        // Pending friend requests
        for (int fromId : data->pendingFrom) {
            QJsonObject req;
            req["type"] = "friend_request";
            req["from_id"] = fromId;
            req["from"] = data->pendingNames.value(fromId);
            sendToSocket(socket, req);
        }

        qDebug() << "User logged in:" << username << "id:" << data->userId;
    });
}

void ChatServer::handleAddFriend(QWebSocket* socket, const QJsonObject& obj)
{
    int fromId = m_socketToUser.value(socket, 0);
    if (fromId == 0) return;

    QString targetName = obj["username"].toString();

    struct Data {
        int toId = 0;
        int status = 0;
        bool requestOk = false;
        QString fromName;
    };
    auto data = std::make_shared<Data>();

    runDbAsync([this, fromId, targetName, data]() {
        data->toId = m_db->getUserId(targetName);
        if (data->toId == 0) return;

        data->status = m_db->getFriendshipStatus(fromId, data->toId);
        if (data->status == 0) {
            data->requestOk = m_db->addFriendRequest(fromId, data->toId);
            data->fromName = m_db->getUsername(fromId);
        }
    }, [this, socket, fromId, targetName, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState)
            return;

        if (data->toId == 0) {
            QJsonObject resp;
            resp["type"] = "add_friend_result";
            resp["success"] = false;
            resp["message"] = "user not found";
            sendToSocket(socket, resp);
            return;
        }
        if (data->status == 2) {
            QJsonObject resp;
            resp["type"] = "add_friend_result";
            resp["success"] = false;
            resp["message"] = "already friends";
            sendToSocket(socket, resp);
            return;
        }
        if (data->status == 1) {
            QJsonObject resp;
            resp["type"] = "add_friend_result";
            resp["success"] = false;
            resp["message"] = "request pending";
            sendToSocket(socket, resp);
            return;
        }
        if (!data->requestOk) {
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

        if (m_userToSocket.contains(data->toId)) {
            QJsonObject req;
            req["type"] = "friend_request";
            req["from_id"] = fromId;
            req["from"] = data->fromName;
            sendToSocket(m_userToSocket[data->toId], req);
        }
    });
}

void ChatServer::handleAcceptFriend(QWebSocket* socket, const QJsonObject& obj)
{
    int toId = m_socketToUser.value(socket, 0);
    if (toId == 0) return;

    QString fromName = obj["username"].toString();

    struct Data {
        int fromId = 0;
        bool ok = false;
        QString toName;
    };
    auto data = std::make_shared<Data>();

    runDbAsync([this, toId, fromName, data]() {
        data->fromId = m_db->getUserId(fromName);
        if (data->fromId == 0) return;
        data->ok = m_db->acceptFriend(data->fromId, toId);
        if (data->ok)
            data->toName = m_db->getUsername(toId);
    }, [this, socket, fromName, toId, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState)
            return;
        if (!data->ok) return;

        QJsonObject resp;
        resp["type"] = "friend_accepted";
        resp["user_id"] = data->fromId;
        resp["username"] = fromName;
        sendToSocket(socket, resp);

        if (m_userToSocket.contains(data->fromId)) {
            QJsonObject notify;
            notify["type"] = "friend_accepted";
            notify["user_id"] = toId;
            notify["username"] = data->toName;
            sendToSocket(m_userToSocket[data->fromId], notify);
        }
    });
}

void ChatServer::handleRejectFriend(QWebSocket* socket, const QJsonObject& obj)
{
    int toId = m_socketToUser.value(socket, 0);
    if (toId == 0) return;

    QString fromName = obj["username"].toString();

    runDbAsync([this, toId, fromName]() {
        int fromId = m_db->getUserId(fromName);
        if (fromId == 0) return;
        m_db->rejectFriend(fromId, toId);
    }, []() {
        // no response needed
    });
}

void ChatServer::handleSendMessage(QWebSocket* socket, const QJsonObject& obj)
{
    int fromId = m_socketToUser.value(socket, 0);
    if (fromId == 0) return;

    QString toName = obj["to"].toString();
    QString content = obj["content"].toString();
    QString time = QDateTime::currentDateTime().toString(Qt::ISODate);

    struct Data {
        int toId = 0;
        int msgId = 0;
        bool ok = false;
        QString fromName;
        bool recipientOnline = false;
    };
    auto data = std::make_shared<Data>();

    runDbAsync([this, fromId, toName, content, time, data]() {
        data->toId = m_db->getUserId(toName);
        if (data->toId == 0) return;

        data->ok = m_db->saveMessage(fromId, data->toId, content, time, &data->msgId);
        if (data->ok)
            data->fromName = m_db->getUsername(fromId);
    }, [this, socket, fromId, toName, content, time, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState)
            return;

        if (data->toId == 0) {
            QJsonObject resp;
            resp["type"] = "error";
            resp["message"] = "user not found";
            sendToSocket(socket, resp);
            return;
        }

        QJsonObject msgToSender;
        msgToSender["type"] = "send_msg_result";
        msgToSender["id"] = data->msgId;
        msgToSender["to_id"] = data->toId;
        msgToSender["to"] = toName;
        msgToSender["content"] = content;
        msgToSender["time"] = time;
        sendToSocket(socket, msgToSender);

        QJsonObject msgToRecipient;
        msgToRecipient["type"] = "recv_msg";
        msgToRecipient["id"] = data->msgId;
        msgToRecipient["from_id"] = fromId;
        msgToRecipient["from"] = data->fromName;
        msgToRecipient["content"] = content;
        msgToRecipient["time"] = time;

        if (m_userToSocket.contains(data->toId)) {
            sendToSocket(m_userToSocket[data->toId], msgToRecipient);
            // Mark delivered on recipient if they're online
            runDbAsync([this, toId = data->toId]() {
                m_db->markDelivered(toId);
            }, []() {});
        }
    });
}

void ChatServer::handleGetFriendList(QWebSocket* socket, const QJsonObject&)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    struct Data {
        QList<int> friendIds;
        QMap<int, QString> names;
    };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->friendIds = m_db->getFriendIds(userId);
        for (int fid : data->friendIds)
            data->names[fid] = m_db->getUsername(fid);
    }, [this, socket, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState)
            return;

        QJsonArray arr;
        for (int fid : data->friendIds) {
            QJsonObject fo;
            fo["user_id"] = fid;
            fo["username"] = data->names.value(fid);
            fo["online"] = m_userToSocket.contains(fid);
            arr.append(fo);
        }

        QJsonObject resp;
        resp["type"] = "friend_list";
        resp["friends"] = arr;
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleGetChatHistory(QWebSocket* socket, const QJsonObject& obj)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    QString friendName = obj["with"].toString();
    int limit = obj["limit"].toInt(30);
    int beforeMsgId = obj["before_msg_id"].toInt(0);

    struct Data {
        int friendId = 0;
        QList<ServerMsg> msgs;
        bool hasMore = false;
        QMap<int, QString> names;
    };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, friendName, limit, beforeMsgId, data]() {
        data->friendId = m_db->getUserId(friendName);
        if (data->friendId == 0) return;

        data->msgs = m_db->getChatHistory(userId, data->friendId, limit + 1, beforeMsgId);
        data->hasMore = data->msgs.size() > limit;
        if (data->hasMore)
            data->msgs.removeFirst();

        for (const auto& m : data->msgs)
            data->names[m.id] = m_db->getUsername(m.fromId);
    }, [this, socket, friendName, userId, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState)
            return;

        QJsonArray arr;
        for (const auto& m : data->msgs) {
            QJsonObject mo;
            mo["id"] = m.id;
            mo["from_id"] = m.fromId;
            mo["from"] = data->names.value(m.id);
            mo["content"] = m.content;
            mo["time"] = m.time;
            mo["is_own"] = (m.fromId == userId);
            arr.append(mo);
        }

        QJsonObject resp;
        resp["type"] = "chat_history";
        resp["with"] = friendName;
        resp["messages"] = arr;
        resp["has_more"] = data->hasMore;

        sendToSocket(socket, resp);
    });
}

// --- Helpers (main thread only) ---

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
    struct Data {
        QList<int> friendIds;
        QString username;
    };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->friendIds = m_db->getFriendIds(userId);
        data->username = m_db->getUsername(userId);
    }, [this, userId, online, data]() {
        QJsonObject notify;
        notify["type"] = "online_status";
        notify["user_id"] = userId;
        notify["username"] = data->username;
        notify["online"] = online;

        for (int fid : data->friendIds) {
            if (m_userToSocket.contains(fid)) {
                sendToSocket(m_userToSocket[fid], notify);
            }
        }
    });
}

// --- Review / Profile handlers ---

void ChatServer::handleSaveReview(QWebSocket* socket, const QJsonObject& obj)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    QString doubanId = obj["douban_id"].toString();
    QString movieName = obj["movie_name"].toString();
    double rating = obj["rating"].toDouble(0);
    QString content = obj["content"].toString();
    bool isWished = obj["is_wished"].toBool(false);
    bool isWatched = obj["is_watched"].toBool(false);
    QString posterUrl = obj["poster_url"].toString();

    struct Data { bool ok = false; ReviewData review; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, doubanId, movieName, rating, content, isWished, isWatched, posterUrl, data]() {
        data->ok = m_db->saveReview(userId, doubanId, movieName, rating, content, isWished, isWatched, posterUrl);
        if (data->ok)
            data->review = m_db->getReview(userId, doubanId);
    }, [this, socket, userId, doubanId, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonObject resp;
        resp["type"] = "review_result";
        resp["success"] = data->ok;
        resp["douban_id"] = doubanId;
        if (data->ok) {
            QJsonObject r;
            r["id"] = data->review.id;
            r["user_id"] = userId;
            r["movie_name"] = data->review.movieName;
            r["rating"] = data->review.rating;
            r["content"] = data->review.content;
            r["is_wished"] = data->review.isWished;
            r["is_watched"] = data->review.isWatched;
            r["poster_url"] = data->review.posterUrl;
            r["create_time"] = data->review.createTime;
            r["update_time"] = data->review.updateTime;
            resp["review"] = r;
        }
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleGetReview(QWebSocket* socket, const QJsonObject& obj)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    QString doubanId = obj["douban_id"].toString();

    struct Data { ReviewData review; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, doubanId, data]() {
        data->review = m_db->getReview(userId, doubanId);
    }, [this, socket, userId, doubanId, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonObject resp;
        resp["type"] = "review_result";
        resp["success"] = data->review.id > 0;
        resp["douban_id"] = doubanId;
        if (data->review.id > 0) {
            QJsonObject r;
            r["id"] = data->review.id;
            r["user_id"] = userId;
            r["douban_id"] = data->review.doubanId;
            r["movie_name"] = data->review.movieName;
            r["rating"] = data->review.rating;
            r["content"] = data->review.content;
            r["is_wished"] = data->review.isWished;
            r["is_watched"] = data->review.isWatched;
            r["poster_url"] = data->review.posterUrl;
            r["create_time"] = data->review.createTime;
            r["update_time"] = data->review.updateTime;
            resp["review"] = r;
        }
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleGetAllReviews(QWebSocket* socket, const QJsonObject&)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    struct Data { QList<ReviewData> reviews; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->reviews = m_db->getAllReviews(userId);
    }, [this, socket, userId, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonArray arr;
        for (const auto& rv : data->reviews) {
            QJsonObject r;
            r["id"] = rv.id;
            r["user_id"] = userId;
            r["douban_id"] = rv.doubanId;
            r["movie_name"] = rv.movieName;
            r["rating"] = rv.rating;
            r["content"] = rv.content;
            r["is_wished"] = rv.isWished;
            r["is_watched"] = rv.isWatched;
            r["poster_url"] = rv.posterUrl;
            r["create_time"] = rv.createTime;
            r["update_time"] = rv.updateTime;
            arr.append(r);
        }
        QJsonObject resp;
        resp["type"] = "reviews_list";
        resp["reviews"] = arr;
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleDeleteReview(QWebSocket* socket, const QJsonObject& obj)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    QString doubanId = obj["douban_id"].toString();

    struct Data { bool ok = false; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, doubanId, data]() {
        data->ok = m_db->deleteReview(userId, doubanId);
    }, [this, socket, doubanId, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonObject resp;
        resp["type"] = "delete_review_result";
        resp["success"] = data->ok;
        resp["douban_id"] = doubanId;
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleGetWishList(QWebSocket* socket, const QJsonObject&)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    struct Data { QList<ReviewData> list; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->list = m_db->getWishList(userId);
    }, [this, socket, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonArray arr;
        for (const auto& rv : data->list) {
            QJsonObject r;
            r["douban_id"] = rv.doubanId;
            r["movie_name"] = rv.movieName;
            r["rating"] = rv.rating;
            r["poster_url"] = rv.posterUrl;
            arr.append(r);
        }
        QJsonObject resp;
        resp["type"] = "wish_list";
        resp["movies"] = arr;
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleGetWatchedList(QWebSocket* socket, const QJsonObject&)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    struct Data { QList<ReviewData> list; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->list = m_db->getWatchedList(userId);
    }, [this, socket, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonArray arr;
        for (const auto& rv : data->list) {
            QJsonObject r;
            r["douban_id"] = rv.doubanId;
            r["movie_name"] = rv.movieName;
            r["rating"] = rv.rating;
            r["poster_url"] = rv.posterUrl;
            arr.append(r);
        }
        QJsonObject resp;
        resp["type"] = "watched_list";
        resp["movies"] = arr;
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleGetProfile(QWebSocket* socket, const QJsonObject&)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    struct Data { ProfileData profile; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->profile = m_db->getProfile(userId);
    }, [this, socket, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonObject resp;
        resp["type"] = "profile_data";
        resp["name"] = data->profile.name;
        resp["bio"] = data->profile.bio;
        resp["avatar_path"] = data->profile.avatarPath;
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleSaveProfile(QWebSocket* socket, const QJsonObject& obj)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    QString name = obj["name"].toString();
    QString bio = obj["bio"].toString();

    struct Data { bool ok = false; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, name, bio, data]() {
        data->ok = m_db->saveProfile(userId, name, bio);
    }, [this, socket, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonObject resp;
        resp["type"] = "profile_saved";
        resp["success"] = data->ok;
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleSaveAvatar(QWebSocket* socket, const QJsonObject& obj)
{
    int userId = m_socketToUser.value(socket, 0);
    if (userId == 0) return;

    QString avatarPath = obj["avatar_path"].toString();

    struct Data { bool ok = false; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, avatarPath, data]() {
        data->ok = m_db->saveAvatarPath(userId, avatarPath);
    }, [this, socket, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonObject resp;
        resp["type"] = "avatar_saved";
        resp["success"] = data->ok;
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleGetMovieReviews(QWebSocket* socket, const QJsonObject& obj)
{
    QString doubanId = obj["douban_id"].toString();
    if (doubanId.isEmpty()) return;

    struct Data { QList<ReviewData> reviews; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, doubanId, data]() {
        data->reviews = m_db->getPublicReviewsByMovie(doubanId);
    }, [this, socket, doubanId, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonArray arr;
        for (const auto& rv : data->reviews) {
            QJsonObject r;
            r["id"] = rv.id;
            r["user_id"] = rv.userId;
            r["username"] = rv.username;
            r["movie_name"] = rv.movieName;
            r["rating"] = rv.rating;
            r["content"] = rv.content;
            r["create_time"] = rv.createTime;
            arr.append(r);
        }
        QJsonObject resp;
        resp["type"] = "movie_reviews";
        resp["douban_id"] = doubanId;
        resp["reviews"] = arr;
        sendToSocket(socket, resp);
    });
}

void ChatServer::handleGetUserReviews(QWebSocket* socket, const QJsonObject& obj)
{
    QString targetName = obj["username"].toString();

    struct Data {
        QList<ReviewData> reviews;
        int targetId = 0;
    };
    auto data = std::make_shared<Data>();

    runDbAsync([this, targetName, data]() {
        data->targetId = m_db->getUserId(targetName);
        if (data->targetId == 0) return;
        data->reviews = m_db->getPublicReviewsByUser(data->targetId);
    }, [this, socket, targetName, data]() {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

        QJsonArray arr;
        for (const auto& rv : data->reviews) {
            QJsonObject r;
            r["id"] = rv.id;
            r["user_id"] = rv.userId;
            r["douban_id"] = rv.doubanId;
            r["movie_name"] = rv.movieName;
            r["rating"] = rv.rating;
            r["content"] = rv.content;
            r["is_wished"] = rv.isWished;
            r["is_watched"] = rv.isWatched;
            r["poster_url"] = rv.posterUrl;
            r["create_time"] = rv.createTime;
            arr.append(r);
        }
        QJsonObject resp;
        resp["type"] = "user_reviews";
        resp["user_id"] = data->targetId;
        resp["username"] = targetName;
        resp["reviews"] = arr;
        sendToSocket(socket, resp);
    });
}
