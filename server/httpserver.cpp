#include "httpserver.h"
#include "serverdb.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QDateTime>
#include <QDebug>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
HttpServer::HttpServer(ServerDb* db, quint16 port, QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_db(db)
{
    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "HTTP server listening on port" << port;
    } else {
        qWarning() << "HTTP server failed to listen on port" << port;
    }

    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
}

// ---------------------------------------------------------------------------
// Connection handling
// ---------------------------------------------------------------------------
void HttpServer::onNewConnection()
{
    QTcpSocket* socket = m_server->nextPendingConnection();
    m_buffers[socket] = QByteArray();
    connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &HttpServer::onDisconnected);
}

void HttpServer::onDisconnected()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    m_buffers.remove(socket);
    socket->deleteLater();
}

void HttpServer::onReadyRead()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray& buf = m_buffers[socket];
    buf.append(socket->readAll());

    bool complete = false;
    Request req = parseRequest(buf, complete);

    if (!complete) return;

    // Remove this request from the buffer (for HTTP/1.1 keep-alive)
    buf.clear();

    dispatch(socket, req);
}

// ---------------------------------------------------------------------------
// HTTP parsing
// ---------------------------------------------------------------------------
HttpServer::Request HttpServer::parseRequest(const QByteArray& raw, bool& complete)
{
    // (bug disabling states for borken in the long term)
    complete = false;
    Request req;

    int headerEnd = raw.indexOf("\r\n\r\n");
    if (headerEnd < 0) return req;

    QByteArray headerPart = raw.left(headerEnd);
    QList<QByteArray> lines = headerPart.split('\n');

    // Request line
    if (lines.isEmpty()) return req;
    QList<QByteArray> reqLine = lines[0].trimmed().split(' ');
    if (reqLine.size() < 2) return req;
    req.method = QString::fromUtf8(reqLine[0]).toUpper();
    QString fullPath = QString::fromUtf8(reqLine[1]);

    // Query string
    int qpos = fullPath.indexOf('?');
    if (qpos >= 0) {
        req.path = fullPath.left(qpos);
        req.query = parseQueryString(fullPath.mid(qpos + 1));
    } else {
        req.path = fullPath;
    }

    // Headers
    for (int i = 1; i < lines.size(); ++i) {
        QByteArray line = lines[i].trimmed();
        int colon = line.indexOf(':');
        if (colon < 0) continue;
        QString key = QString::fromUtf8(line.left(colon)).trimmed().toLower();
        QString val = QString::fromUtf8(line.mid(colon + 1)).trimmed();
        req.headers[key] = val;
    }

    // Body
    int contentLen = req.headers.value("content-length", "0").toInt();
    int bodyStart = headerEnd + 4;
    if (raw.size() - bodyStart < contentLen) return req; // incomplete

    req.body = raw.mid(bodyStart, contentLen);
    complete = true;
    return req;
}

QMap<QString, QString> HttpServer::parseQueryString(const QString& query) const
{
    QMap<QString, QString> map;
    QStringList pairs = query.split('&', Qt::SkipEmptyParts);
    for (const QString& pair : pairs) {
        int eq = pair.indexOf('=');
        if (eq >= 0)
            map[pair.left(eq)] = QUrl::fromPercentEncoding(pair.mid(eq + 1).toUtf8());
        else
            map[pair] = QString();
    }
    return map;
}

bool HttpServer::parseMultipart(const QByteArray& body, const QString& boundary,
                                QMap<QString, QByteArray>& fields,
                                QMap<QString, QByteArray>& files)
{
    QByteArray delim = ("--" + boundary).toUtf8();
    QByteArray endDelim = ("--" + boundary + "--").toUtf8();

    int pos = body.indexOf(delim);
    if (pos < 0) return false;

    while (pos >= 0) {
        pos += delim.size();
        if (body.mid(pos, 2) == "\r\n") pos += 2;
        else if (body.mid(pos, 1) == "\n") pos += 1;
        else break; // end delimiter

        int nextDelim = body.indexOf(delim, pos);
        if (nextDelim < 0) break;

        QByteArray part = body.mid(pos, nextDelim - pos);

        // Trim trailing \r\n before delimiter
        if (part.endsWith("\r\n")) part.chop(2);
        else if (part.endsWith("\n")) part.chop(1);

        // Split headers and body
        int partHeaderEnd = part.indexOf("\r\n\r\n");
        if (partHeaderEnd < 0) partHeaderEnd = part.indexOf("\n\n");
        if (partHeaderEnd < 0) { pos = nextDelim; continue; }

        int partBodyStart = partHeaderEnd;
        if (part.mid(partHeaderEnd, 4) == "\r\n\r\n") partBodyStart = partHeaderEnd + 4;
        else partBodyStart = partHeaderEnd + 2;

        QByteArray partHeaders = part.left(partHeaderEnd);
        QByteArray partBody = part.mid(partBodyStart);

        // Parse Content-Disposition for name and filename
        QString headerStr = QString::fromUtf8(partHeaders);
        QString name;
        QString filename;

        // Extract name=
        int nameIdx = headerStr.indexOf("name=\"");
        if (nameIdx >= 0) {
            nameIdx += 6;
            int nameEnd = headerStr.indexOf('"', nameIdx);
            if (nameEnd >= 0) name = headerStr.mid(nameIdx, nameEnd - nameIdx);
        }

        // Extract filename=
        int fnIdx = headerStr.indexOf("filename=\"");
        if (fnIdx >= 0) {
            fnIdx += 10;
            int fnEnd = headerStr.indexOf('"', fnIdx);
            if (fnEnd >= 0) filename = headerStr.mid(fnIdx, fnEnd - fnIdx);
        }

        if (!name.isEmpty()) {
            if (!filename.isEmpty())
                files[name] = partBody;
            else
                fields[name] = partBody;
        }

        pos = nextDelim;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------
void HttpServer::dispatch(QTcpSocket* socket, const Request& req)
{
    // ---------- POST ----------
    if (req.method == "POST") {
        if (req.path == "/api/login")            { handleLogin(socket, req); return; }
        if (req.path == "/api/register")         { handleRegister(socket, req); return; }
        if (req.path == "/api/friends/add")      { handleAddFriend(socket, req); return; }
        if (req.path == "/api/friends/accept")   { handleAcceptFriend(socket, req); return; }
        if (req.path == "/api/friends/reject")   { handleRejectFriend(socket, req); return; }
        if (req.path == "/api/profile")           { handleSaveProfile(socket, req); return; }
        if (req.path == "/api/avatar")            { handleUploadAvatar(socket, req); return; }
        if (req.path == "/api/reviews")           { handleSaveReview(socket, req); return; }
    }

    // ---------- PUT ----------
    if (req.method == "PUT") {
        if (req.path == "/api/profile") { handleSaveProfile(socket, req); return; }
    }

    // ---------- DELETE ----------
    if (req.method == "DELETE") {
        if (req.path == "/api/reviews") { handleDeleteReview(socket, req); return; }
    }

    // ---------- GET ----------
    if (req.method == "GET") {
        if (req.path == "/api/friends")           { handleGetFriends(socket, req); return; }
        if (req.path == "/api/chat/history")      { handleChatHistory(socket, req); return; }
        if (req.path == "/api/profile")           { handleGetProfile(socket, req); return; }
        if (req.path == "/api/wishlist")          { handleGetWishList(socket, req); return; }
        if (req.path == "/api/watched")           { handleGetWatchedList(socket, req); return; }

        // Path-parameter routes
        if (req.path.startsWith("/api/avatar/"))  { handleGetAvatar(socket, req); return; }
        if (req.path.startsWith("/api/reviews/movie/")) { handleGetMovieReviews(socket, req); return; }
        if (req.path.startsWith("/api/reviews/user"))   { handleGetUserReviews(socket, req); return; }
    }

    sendError(socket, 404, "Not Found");
}

// ---------------------------------------------------------------------------
// Response helpers
// ---------------------------------------------------------------------------
void HttpServer::sendJson(QTcpSocket* socket, int statusCode, const QJsonObject& obj)
{
    if (!socket || socket->state() != QTcpSocket::ConnectedState) return;

    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray resp;
    resp += "HTTP/1.1 " + QByteArray::number(statusCode) + " ";
    resp += (statusCode == 200 ? "OK" : (statusCode == 201 ? "Created" :
             (statusCode == 400 ? "Bad Request" : (statusCode == 401 ? "Unauthorized" :
             (statusCode == 404 ? "Not Found" : "Error")))));
    resp += "\r\n";
    resp += "Content-Type: application/json\r\n";
    resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += body;

    socket->write(resp);
    socket->flush();
    discardSocket(socket);
}

void HttpServer::sendBinary(QTcpSocket* socket, const QByteArray& data, const QString& contentType)
{
    if (!socket || socket->state() != QTcpSocket::ConnectedState) return;

    QByteArray resp;
    resp += "HTTP/1.1 200 OK\r\n";
    resp += "Content-Type: " + contentType.toUtf8() + "\r\n";
    resp += "Content-Length: " + QByteArray::number(data.size()) + "\r\n";
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += data;

    socket->write(resp);
    socket->flush();
    discardSocket(socket);
}

void HttpServer::sendError(QTcpSocket* socket, int statusCode, const QString& msg)
{
    QJsonObject obj;
    obj["success"] = false;
    obj["error"] = msg;
    sendJson(socket, statusCode, obj);
}

void HttpServer::discardSocket(QTcpSocket* socket)
{
    // HTTP/1.1 Connection: close — schedule shutdown after flush
    connect(socket, &QTcpSocket::bytesWritten, socket, [socket](qint64) {
        if (socket->bytesToWrite() == 0) {
            socket->disconnectFromHost();
            socket->deleteLater();
        }
    });
}

// ---------------------------------------------------------------------------
// POST /api/login
// ---------------------------------------------------------------------------
void HttpServer::handleLogin(QTcpSocket* socket, const Request& req)
{
    QJsonObject body = QJsonDocument::fromJson(req.body).object();
    QString username = body["username"].toString();
    QString password = body["password"].toString();

    if (username.isEmpty() || password.isEmpty()) {
        sendError(socket, 400, "username and password required");
        return;
    }

    struct Data { int userId = 0; QString name; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, username, password, data]() {
        data->userId = m_db->loginUser(username, password);
        if (data->userId == 0)
            data->userId = m_db->registerUser(username, password);
        if (data->userId > 0)
            data->name = m_db->getUsername(data->userId);
    }, [this, socket, data]() {
        QJsonObject resp;
        if (data->userId == 0) {
            resp["success"] = false;
            resp["error"] = "login failed";
            sendJson(socket, 401, resp);
        } else {
            resp["success"] = true;
            resp["user_id"] = data->userId;
            resp["username"] = data->name;
            sendJson(socket, 200, resp);
        }
    });
}

// ---------------------------------------------------------------------------
// POST /api/register
// ---------------------------------------------------------------------------
void HttpServer::handleRegister(QTcpSocket* socket, const Request& req)
{
    QJsonObject body = QJsonDocument::fromJson(req.body).object();
    QString username = body["username"].toString();
    QString password = body["password"].toString();

    if (username.isEmpty() || password.isEmpty()) {
        sendError(socket, 400, "username and password required");
        return;
    }

    struct Data { int userId = 0; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, username, password, data]() {
        data->userId = m_db->registerUser(username, password);
    }, [this, socket, username, data]() {
        QJsonObject resp;
        if (data->userId == 0) {
            resp["success"] = false;
            resp["error"] = "username already taken";
            sendJson(socket, 409, resp);
        } else {
            resp["success"] = true;
            resp["user_id"] = data->userId;
            resp["username"] = username;
            sendJson(socket, 201, resp);
        }
    });
}

// ---------------------------------------------------------------------------
// GET /api/friends?user_id=
// ---------------------------------------------------------------------------
void HttpServer::handleGetFriends(QTcpSocket* socket, const Request& req)
{
    int userId = req.query.value("user_id", "0").toInt();
    if (userId == 0) { sendError(socket, 400, "user_id required"); return; }

    struct Data { QList<int> ids; QMap<int, QString> names; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->ids = m_db->getFriendIds(userId);
        for (int fid : data->ids)
            data->names[fid] = m_db->getUsername(fid);
    }, [this, socket, data]() {
        QJsonArray arr;
        for (int fid : data->ids) {
            QJsonObject fo;
            fo["user_id"] = fid;
            fo["username"] = data->names.value(fid);
            arr.append(fo);
        }
        QJsonObject resp;
        resp["success"] = true;
        resp["friends"] = arr;
        sendJson(socket, 200, resp);
    });
}

// ---------------------------------------------------------------------------
// POST /api/friends/add   body: { user_id, username }
// ---------------------------------------------------------------------------
void HttpServer::handleAddFriend(QTcpSocket* socket, const Request& req)
{
    QJsonObject body = QJsonDocument::fromJson(req.body).object();
    int fromId = body["user_id"].toInt();
    QString targetName = body["username"].toString();

    if (fromId == 0 || targetName.isEmpty()) {
        sendError(socket, 400, "user_id and username required");
        return;
    }

    struct Data { int toId = 0; int status = 0; bool ok = false; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, fromId, targetName, data]() {
        data->toId = m_db->getUserId(targetName);
        if (data->toId == 0) return;
        data->status = m_db->getFriendshipStatus(fromId, data->toId);
        if (data->status == 0)
            data->ok = m_db->addFriendRequest(fromId, data->toId);
    }, [this, socket, data]() {
        if (data->toId == 0) {
            sendError(socket, 404, "user not found");
        } else if (data->status == 2) {
            sendError(socket, 400, "already friends");
        } else if (data->status == 1) {
            sendError(socket, 400, "request already pending");
        } else if (!data->ok) {
            sendError(socket, 500, "request failed");
        } else {
            QJsonObject resp;
            resp["success"] = true;
            resp["message"] = "friend request sent";
            sendJson(socket, 200, resp);
        }
    });
}

// ---------------------------------------------------------------------------
// POST /api/friends/accept   body: { user_id, username }
// ---------------------------------------------------------------------------
void HttpServer::handleAcceptFriend(QTcpSocket* socket, const Request& req)
{
    QJsonObject body = QJsonDocument::fromJson(req.body).object();
    int toId = body["user_id"].toInt();
    QString fromName = body["username"].toString();

    if (toId == 0 || fromName.isEmpty()) {
        sendError(socket, 400, "user_id and username required");
        return;
    }

    struct Data { int fromId = 0; bool ok = false; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, toId, fromName, data]() {
        data->fromId = m_db->getUserId(fromName);
        if (data->fromId == 0) return;
        data->ok = m_db->acceptFriend(data->fromId, toId);
    }, [this, socket, data]() {
        QJsonObject resp;
        resp["success"] = data->ok;
        if (!data->ok) resp["error"] = "accept failed";
        sendJson(socket, data->ok ? 200 : 400, resp);
    });
}

// ---------------------------------------------------------------------------
// POST /api/friends/reject   body: { user_id, username }
// ---------------------------------------------------------------------------
void HttpServer::handleRejectFriend(QTcpSocket* socket, const Request& req)
{
    QJsonObject body = QJsonDocument::fromJson(req.body).object();
    int toId = body["user_id"].toInt();
    QString fromName = body["username"].toString();

    if (toId == 0 || fromName.isEmpty()) {
        sendError(socket, 400, "user_id and username required");
        return;
    }

    runDbAsync([this, toId, fromName]() {
        int fromId = m_db->getUserId(fromName);
        if (fromId == 0) return;
        m_db->rejectFriend(fromId, toId);
    }, [this, socket]() {
        QJsonObject resp;
        resp["success"] = true;
        sendJson(socket, 200, resp);
    });
}

// ---------------------------------------------------------------------------
// GET /api/chat/history?user_id=&with=&limit=&before=
// ---------------------------------------------------------------------------
void HttpServer::handleChatHistory(QTcpSocket* socket, const Request& req)
{
    int userId = req.query.value("user_id", "0").toInt();
    QString with = req.query.value("with");
    int limit = req.query.value("limit", "30").toInt();
    int before = req.query.value("before", "0").toInt();

    if (userId == 0 || with.isEmpty()) {
        sendError(socket, 400, "user_id and with required");
        return;
    }

    struct Data {
        int friendId = 0;
        QList<ServerMsg> msgs;
        bool hasMore = false;
        QMap<int, QString> names;
    };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, with, limit, before, data]() {
        data->friendId = m_db->getUserId(with);
        if (data->friendId == 0) return;
        data->msgs = m_db->getChatHistory(userId, data->friendId, limit + 1, before);
        data->hasMore = data->msgs.size() > limit;
        if (data->hasMore) data->msgs.removeFirst();
        for (const auto& m : data->msgs)
            data->names[m.id] = m_db->getUsername(m.fromId);
    }, [this, socket, with, userId, data]() {
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
        resp["success"] = true;
        resp["with"] = with;
        resp["messages"] = arr;
        resp["has_more"] = data->hasMore;
        sendJson(socket, 200, resp);
    });
}

// ---------------------------------------------------------------------------
// GET /api/profile?user_id=
// ---------------------------------------------------------------------------
void HttpServer::handleGetProfile(QTcpSocket* socket, const Request& req)
{
    int userId = req.query.value("user_id", "0").toInt();
    if (userId == 0) { sendError(socket, 400, "user_id required"); return; }

    struct Data { ProfileData profile; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->profile = m_db->getProfile(userId);
    }, [this, socket, data]() {
        QJsonObject resp;
        resp["success"] = true;
        resp["name"] = data->profile.name;
        resp["bio"] = data->profile.bio;
        resp["avatar_path"] = data->profile.avatarPath;
        sendJson(socket, 200, resp);
    });
}

// ---------------------------------------------------------------------------
// PUT /api/profile   body: { user_id, name, bio }
// ---------------------------------------------------------------------------
void HttpServer::handleSaveProfile(QTcpSocket* socket, const Request& req)
{
    QJsonObject body = QJsonDocument::fromJson(req.body).object();
    int userId = body["user_id"].toInt();
    QString name = body["name"].toString();
    QString bio = body["bio"].toString();

    if (userId == 0) { sendError(socket, 400, "user_id required"); return; }

    struct Data { bool ok = false; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, name, bio, data]() {
        data->ok = m_db->saveProfile(userId, name, bio);
    }, [this, socket, data]() {
        QJsonObject resp;
        resp["success"] = data->ok;
        sendJson(socket, data->ok ? 200 : 500, resp);
    });
}

// ---------------------------------------------------------------------------
// POST /api/avatar   multipart fields: user_id, file
// ---------------------------------------------------------------------------
void HttpServer::handleUploadAvatar(QTcpSocket* socket, const Request& req)
{
    QString ctype = req.headers.value("content-type");
    QString boundary;
    int bi = ctype.indexOf("boundary=");
    if (bi >= 0) {
        boundary = ctype.mid(bi + 9).trimmed();
        // Qt QHttpMultiPart quotes the boundary value
        if (boundary.startsWith('"') && boundary.endsWith('"'))
            boundary = boundary.mid(1, boundary.size() - 2);
    }

    if (boundary.isEmpty()) {
        sendError(socket, 400, "multipart/form-data required");
        return;
    }

    QMap<QString, QByteArray> fields;
    QMap<QString, QByteArray> files;
    if (!parseMultipart(req.body, boundary, fields, files)) {
        sendError(socket, 400, "failed to parse multipart");
        return;
    }

    int userId = fields.value("user_id", "0").toInt();
    if (userId == 0 || !files.contains("file")) {
        sendError(socket, 400, "user_id and file required");
        return;
    }

    // Save avatar to disk
    QDir().mkpath("avatars");
    QString filePath = QString("avatars/%1.png").arg(userId);
    QFile f(filePath);
    f.open(QIODevice::WriteOnly);
    f.write(files["file"]);
    f.close();

    // Save path to DB
    struct Data { bool ok = false; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, filePath, data]() {
        data->ok = m_db->saveAvatarPath(userId, filePath);
    }, [this, socket, userId, data]() {
        QJsonObject resp;
        resp["success"] = data->ok;
        resp["avatar_path"] = QString("avatars/%1.png").arg(userId);
        sendJson(socket, data->ok ? 200 : 500, resp);
    });
}

// ---------------------------------------------------------------------------
// GET /api/avatar/{userId}
// ---------------------------------------------------------------------------
void HttpServer::handleGetAvatar(QTcpSocket* socket, const Request& req)
{
    // Extract userId from last path segment
    QStringList segs = req.path.split('/');
    if (segs.isEmpty()) { sendError(socket, 404, "not found"); return; }
    int userId = segs.last().toInt();
    if (userId == 0) { sendError(socket, 404, "not found"); return; }

    struct Data { ProfileData profile; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->profile = m_db->getProfile(userId);
    }, [this, socket, data]() {
        QString path = data->profile.avatarPath;
        if (path.isEmpty() || !QFile::exists(path)) {
            sendError(socket, 404, "avatar not found");
            return;
        }
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            sendError(socket, 404, "cannot read avatar");
            return;
        }
        QByteArray imgData = f.readAll();
        f.close();

        // Determine content type from extension
        QString ct = "image/png";
        if (path.endsWith(".jpg") || path.endsWith(".jpeg")) ct = "image/jpeg";
        else if (path.endsWith(".gif")) ct = "image/gif";
        else if (path.endsWith(".webp")) ct = "image/webp";

        sendBinary(socket, imgData, ct);
    });
}

// ---------------------------------------------------------------------------
// GET /api/reviews/movie/{doubanId}
// ---------------------------------------------------------------------------
void HttpServer::handleGetMovieReviews(QTcpSocket* socket, const Request& req)
{
    QStringList segs = req.path.split('/');
    if (segs.size() < 4) { sendError(socket, 404, "not found"); return; }
    QString doubanId = segs.last();
    if (doubanId.isEmpty()) { sendError(socket, 400, "douban_id required"); return; }

    struct Data { QList<ReviewData> reviews; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, doubanId, data]() {
        data->reviews = m_db->getPublicReviewsByMovie(doubanId);
    }, [this, socket, doubanId, data]() {
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
        resp["success"] = true;
        resp["douban_id"] = doubanId;
        resp["reviews"] = arr;
        sendJson(socket, 200, resp);
    });
}

// ---------------------------------------------------------------------------
// GET /api/reviews/user?user_id=
// ---------------------------------------------------------------------------
void HttpServer::handleGetUserReviews(QTcpSocket* socket, const Request& req)
{
    int userId = req.query.value("user_id", "0").toInt();
    if (userId == 0) { sendError(socket, 400, "user_id required"); return; }

    struct Data { QList<ReviewData> reviews; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->reviews = m_db->getPublicReviewsByUser(userId);
    }, [this, socket, userId, data]() {
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
        resp["success"] = true;
        resp["user_id"] = userId;
        resp["reviews"] = arr;
        sendJson(socket, 200, resp);
    });
}

// ---------------------------------------------------------------------------
// POST /api/reviews   body: { user_id, douban_id, movie_name, rating, content, ... }
// ---------------------------------------------------------------------------
void HttpServer::handleSaveReview(QTcpSocket* socket, const Request& req)
{
    QJsonObject body = QJsonDocument::fromJson(req.body).object();
    int userId = body["user_id"].toInt();
    QString doubanId = body["douban_id"].toString();
    QString movieName = body["movie_name"].toString();
    double rating = body["rating"].toDouble(0);
    QString content = body["content"].toString();
    bool isWished = body["is_wished"].toBool(false);
    bool isWatched = body["is_watched"].toBool(false);
    QString posterUrl = body["poster_url"].toString();

    if (userId == 0 || doubanId.isEmpty()) {
        sendError(socket, 400, "user_id and douban_id required");
        return;
    }

    struct Data { bool ok = false; ReviewData review; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, doubanId, movieName, rating, content, isWished, isWatched, posterUrl, data]() {
        data->ok = m_db->saveReview(userId, doubanId, movieName, rating, content, isWished, isWatched, posterUrl);
        if (data->ok)
            data->review = m_db->getReview(userId, doubanId);
    }, [this, socket, userId, data]() {
        QJsonObject resp;
        resp["success"] = data->ok;
        if (data->ok) {
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
        } else {
            resp["error"] = "save failed";
        }
        sendJson(socket, data->ok ? 200 : 500, resp);
    });
}

// ---------------------------------------------------------------------------
// DELETE /api/reviews   body: { user_id, douban_id }
// ---------------------------------------------------------------------------
void HttpServer::handleDeleteReview(QTcpSocket* socket, const Request& req)
{
    QJsonObject body = QJsonDocument::fromJson(req.body).object();
    int userId = body["user_id"].toInt();
    QString doubanId = body["douban_id"].toString();

    if (userId == 0 || doubanId.isEmpty()) {
        sendError(socket, 400, "user_id and douban_id required");
        return;
    }

    struct Data { bool ok = false; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, doubanId, data]() {
        data->ok = m_db->deleteReview(userId, doubanId);
    }, [this, socket, data]() {
        QJsonObject resp;
        resp["success"] = data->ok;
        sendJson(socket, data->ok ? 200 : 404, resp);
    });
}

// ---------------------------------------------------------------------------
// GET /api/wishlist?user_id=
// ---------------------------------------------------------------------------
void HttpServer::handleGetWishList(QTcpSocket* socket, const Request& req)
{
    int userId = req.query.value("user_id", "0").toInt();
    if (userId == 0) { sendError(socket, 400, "user_id required"); return; }

    struct Data { QList<ReviewData> list; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->list = m_db->getWishList(userId);
    }, [this, socket, data]() {
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
        resp["success"] = true;
        resp["movies"] = arr;
        sendJson(socket, 200, resp);
    });
}

// ---------------------------------------------------------------------------
// GET /api/watched?user_id=
// ---------------------------------------------------------------------------
void HttpServer::handleGetWatchedList(QTcpSocket* socket, const Request& req)
{
    int userId = req.query.value("user_id", "0").toInt();
    if (userId == 0) { sendError(socket, 400, "user_id required"); return; }

    struct Data { QList<ReviewData> list; };
    auto data = std::make_shared<Data>();

    runDbAsync([this, userId, data]() {
        data->list = m_db->getWatchedList(userId);
    }, [this, socket, data]() {
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
        resp["success"] = true;
        resp["movies"] = arr;
        sendJson(socket, 200, resp);
    });
}
