#include "serverapiclient.h"
#include <QJsonDocument>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>
#include <QUrlQuery>
#include <QDebug>

ServerApiClient::ServerApiClient(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void ServerApiClient::setBaseUrl(const QString& url)
{
    m_baseUrl = url;
}

// ---------------------------------------------------------------------------
// HTTP helpers
// ---------------------------------------------------------------------------
QNetworkReply* ServerApiClient::post(const QString& path, const QJsonObject& body)
{
    QNetworkRequest req(QUrl(m_baseUrl + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

QNetworkReply* ServerApiClient::put(const QString& path, const QJsonObject& body)
{
    QNetworkRequest req(QUrl(m_baseUrl + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return m_nam->put(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

QNetworkReply* ServerApiClient::del(const QString& path, const QJsonObject& body)
{
    QNetworkRequest req(QUrl(m_baseUrl + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return m_nam->sendCustomRequest(req, "DELETE", QJsonDocument(body).toJson(QJsonDocument::Compact));
}

QNetworkReply* ServerApiClient::get(const QString& path)
{
    QNetworkRequest req(QUrl(m_baseUrl + path));
    return m_nam->get(req);
}

// ---------------------------------------------------------------------------
// registerUser
// ---------------------------------------------------------------------------
void ServerApiClient::registerUser(const QString& username, const QString& password)
{
    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    QNetworkReply* reply = post("/api/register", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        emit registerResult(resp["success"].toBool(),
                            resp["user_id"].toInt(),
                            resp["error"].toString());
    });
}

// ---------------------------------------------------------------------------
// Friends
// ---------------------------------------------------------------------------
void ServerApiClient::getFriendList(int userId)
{
    QNetworkReply* reply = get(QString("/api/friends?user_id=%1").arg(userId));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        QList<FriendInfo> friends;
        QJsonArray arr = resp["friends"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject fo = v.toObject();
            FriendInfo fi;
            fi.userId = fo["user_id"].toInt();
            fi.username = fo["username"].toString();
            fi.online = false; // HTTP can't know online status
            friends.append(fi);
        }
        emit friendListReceived(friends);
    });
}

void ServerApiClient::addFriend(int userId, const QString& targetUsername)
{
    QJsonObject body;
    body["user_id"] = userId;
    body["username"] = targetUsername;
    QNetworkReply* reply = post("/api/friends/add", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        emit addFriendResult(resp["success"].toBool(), resp["message"].toString());
    });
}

void ServerApiClient::acceptFriend(int userId, const QString& fromUsername)
{
    QJsonObject body;
    body["user_id"] = userId;
    body["username"] = fromUsername;
    QNetworkReply* reply = post("/api/friends/accept", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        emit acceptFriendResult(resp["success"].toBool());
    });
}

void ServerApiClient::rejectFriend(int userId, const QString& fromUsername)
{
    QJsonObject body;
    body["user_id"] = userId;
    body["username"] = fromUsername;
    QNetworkReply* reply = post("/api/friends/reject", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        emit rejectFriendResult(resp["success"].toBool());
    });
}

// ---------------------------------------------------------------------------
// Chat history
// ---------------------------------------------------------------------------
void ServerApiClient::getChatHistory(int userId, const QString& withUser, int limit, int beforeMsgId)
{
    QString path = QString("/api/chat/history?user_id=%1&with=%2&limit=%3&before=%4")
                       .arg(userId).arg(withUser).arg(limit).arg(beforeMsgId);
    QNetworkReply* reply = get(path);
    connect(reply, &QNetworkReply::finished, this, [this, reply, withUser, userId]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        bool hasMore = resp["has_more"].toBool();
        QList<ChatMsg> messages;
        QJsonArray arr = resp["messages"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject mo = v.toObject();
            ChatMsg msg;
            msg.id = mo["id"].toInt();
            msg.fromId = mo["from_id"].toInt();
            msg.from = mo["from"].toString();
            msg.content = mo["content"].toString();
            msg.time = mo["time"].toString();
            msg.isOwn = mo["is_own"].toBool();
            msg.to = msg.isOwn ? withUser : QString();
            messages.append(msg);
        }
        emit chatHistoryReceived(withUser, messages, hasMore);
    });
}

// ---------------------------------------------------------------------------
// Profile
// ---------------------------------------------------------------------------
void ServerApiClient::getProfile(int userId)
{
    QNetworkReply* reply = get(QString("/api/profile?user_id=%1").arg(userId));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        emit profileReceived(resp["name"].toString(),
                             resp["bio"].toString(),
                             resp["avatar_path"].toString());
    });
}

void ServerApiClient::saveProfile(int userId, const QString& name, const QString& bio)
{
    QJsonObject body;
    body["user_id"] = userId;
    body["name"] = name;
    body["bio"] = bio;
    QNetworkReply* reply = put("/api/profile", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        emit profileSaved(resp["success"].toBool());
    });
}

void ServerApiClient::uploadAvatar(int userId, const QString& filePath)
{
    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        emit avatarSaved(false);
        return;
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // user_id field
    QHttpPart userIdPart;
    userIdPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                         QVariant("form-data; name=\"user_id\""));
    userIdPart.setBody(QByteArray::number(userId));
    multiPart->append(userIdPart);

    // file field
    QHttpPart filePart;
    QString fileName = QFileInfo(filePath).fileName();
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(fileName)));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    filePart.setBodyDevice(file);
    file->setParent(multiPart); // multiPart takes ownership
    multiPart->append(filePart);

    QNetworkRequest req(QUrl(m_baseUrl + "/api/avatar"));
    QNetworkReply* reply = m_nam->post(req, multiPart);
    multiPart->setParent(reply); // reply takes ownership

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        emit avatarSaved(resp["success"].toBool());
    });
}

// ---------------------------------------------------------------------------
// Reviews
// ---------------------------------------------------------------------------
void ServerApiClient::saveReview(int userId, const QString& doubanId, const QString& movieName,
                                  double rating, const QString& content, bool isWished, bool isWatched,
                                  const QString& posterUrl)
{
    QJsonObject body;
    body["user_id"] = userId;
    body["douban_id"] = doubanId;
    body["movie_name"] = movieName;
    body["rating"] = rating;
    body["content"] = content;
    body["is_wished"] = isWished;
    body["is_watched"] = isWatched;
    body["poster_url"] = posterUrl;

    QNetworkReply* reply = post("/api/reviews", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply, doubanId]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        bool success = resp["success"].toBool();
        if (success && resp.contains("review")) {
            QJsonObject r = resp["review"].toObject();
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
    });
}

void ServerApiClient::getReview(int userId, const QString& doubanId)
{
    // Reuse getAllReviews and filter, or add a dedicated endpoint
    // For now, use getAllReviews as the server doesn't have a single-review GET endpoint
    QNetworkReply* reply = get(QString("/api/reviews/user?user_id=%1").arg(userId));
    connect(reply, &QNetworkReply::finished, this, [this, reply, doubanId]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        QJsonArray arr = resp["reviews"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject r = v.toObject();
            if (r["douban_id"].toString() == doubanId) {
                UserReview review;
                review.id = r["id"].toInt();
                review.doubanId = r["douban_id"].toString();
                review.movieName = r["movie_name"].toString();
                review.rating = r["rating"].toDouble();
                review.content = r["content"].toString();
                review.isWished = r["is_wished"].toBool();
                review.isWatched = r["is_watched"].toBool();
                review.posterUrl = r["poster_url"].toString();
                review.createTime = r["create_time"].toString();
                emit reviewReceived(review);
                return;
            }
        }
        emit reviewSaved(false, doubanId);
    });
}

void ServerApiClient::getAllReviews(int userId)
{
    QNetworkReply* reply = get(QString("/api/reviews/user?user_id=%1").arg(userId));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        QList<UserReview> list;
        QJsonArray arr = resp["reviews"].toArray();
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
    });
}

void ServerApiClient::deleteReview(int userId, const QString& doubanId)
{
    QJsonObject body;
    body["user_id"] = userId;
    body["douban_id"] = doubanId;
    QNetworkReply* reply = del("/api/reviews", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply, doubanId]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        emit reviewDeleted(resp["success"].toBool(), doubanId);
    });
}

void ServerApiClient::getWishList(int userId)
{
    QNetworkReply* reply = get(QString("/api/wishlist?user_id=%1").arg(userId));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        QList<UserReview> list;
        QJsonArray arr = resp["movies"].toArray();
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
    });
}

void ServerApiClient::getWatchedList(int userId)
{
    QNetworkReply* reply = get(QString("/api/watched?user_id=%1").arg(userId));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        QList<UserReview> list;
        QJsonArray arr = resp["movies"].toArray();
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
    });
}

void ServerApiClient::getMovieReviews(const QString& doubanId)
{
    QNetworkReply* reply = get(QString("/api/reviews/movie/%1").arg(doubanId));
    connect(reply, &QNetworkReply::finished, this, [this, reply, doubanId]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        QList<UserReview> list;
        QJsonArray arr = resp["reviews"].toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject r = v.toObject();
            UserReview review;
            review.id = r["id"].toInt();
            review.userId = r["user_id"].toInt();
            review.username = r["username"].toString();
            review.movieName = r["movie_name"].toString();
            review.rating = r["rating"].toDouble();
            review.content = r["content"].toString();
            review.createTime = r["create_time"].toString();
            review.doubanId = doubanId;
            list.append(review);
        }
        emit movieReviewsReceived(doubanId, list);
    });
}

void ServerApiClient::getUserReviews(int userId)
{
    QNetworkReply* reply = get(QString("/api/reviews/user?user_id=%1").arg(userId));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        QList<UserReview> list;
        QJsonArray arr = resp["reviews"].toArray();
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
        // Use empty username — caller knows whose reviews they are
        emit userReviewsReceived(QString(), list);
    });
}
