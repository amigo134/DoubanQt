#include "serverdb.h"
#include "connectionpool.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

ServerDb::ServerDb(int poolSize, QObject* parent)
    : QObject(parent)
    , m_pool(new ConnectionPool(poolSize, this))
{
}

ServerDb::~ServerDb()
{
}

bool ServerDb::initialize()
{
    // Ensure database and tables exist with a temp connection
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "init_conn");
        db.setHostName("localhost");
        db.setPort(3306);
        db.setUserName("root");
        db.setPassword("123456");
        db.setDatabaseName("chatserver");

        if (!db.open()) {
            db.setDatabaseName("");
            if (!db.open()) {
                qWarning() << "Connect to MySQL failed:" << db.lastError().text();
                QSqlDatabase::removeDatabase("init_conn");
                return false;
            }
            QSqlQuery q(db);
            q.exec("CREATE DATABASE IF NOT EXISTS `chatserver` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci");
            db.close();
            db.setDatabaseName("chatserver");
            if (!db.open()) {
                qWarning() << "MySQL DB open failed:" << db.lastError().text();
                QSqlDatabase::removeDatabase("init_conn");
                return false;
            }
        }

        if (!createTables(db)) {
            QSqlDatabase::removeDatabase("init_conn");
            return false;
        }
        QSqlDatabase::removeDatabase("init_conn");
    }

    // Now open the pool
    if (!m_pool->initialize("localhost", 3306, "root", "123456", "chatserver")) {
        qWarning() << "Connection pool init failed";
        return false;
    }

    return true;
}

bool ServerDb::createTables(const QSqlDatabase& db)
{
    QSqlQuery q(db);

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INT PRIMARY KEY AUTO_INCREMENT,
            username VARCHAR(64) UNIQUE NOT NULL,
            password VARCHAR(128) NOT NULL,
            created_at DATETIME
        )
    )");

    bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS friendships (
            id INT PRIMARY KEY AUTO_INCREMENT,
            from_id INT NOT NULL,
            to_id INT NOT NULL,
            status VARCHAR(16) DEFAULT 'pending',
            created_at DATETIME,
            UNIQUE(from_id, to_id)
        )
    )");

    if (!ok) {
        qWarning() << "Create friendships failed:" << q.lastError().text();
        return false;
    }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INT PRIMARY KEY AUTO_INCREMENT,
            from_id INT NOT NULL,
            to_id INT NOT NULL,
            content TEXT,
            time DATETIME,
            delivered TINYINT DEFAULT 0
        )
    )");

    if (!ok) {
        qWarning() << "Create messages failed:" << q.lastError().text();
        return false;
    }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS reviews (
            id INT PRIMARY KEY AUTO_INCREMENT,
            user_id INT NOT NULL,
            douban_id VARCHAR(32) NOT NULL,
            movie_name TEXT,
            rating REAL DEFAULT 0,
            content TEXT,
            is_wished TINYINT DEFAULT 0,
            is_watched TINYINT DEFAULT 0,
            poster_url TEXT,
            create_time DATETIME,
            update_time DATETIME,
            UNIQUE KEY uk_user_douban (user_id, douban_id)
        )
    )");

    if (!ok) {
        qWarning() << "Create reviews failed:" << q.lastError().text();
        return false;
    }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS user_profile (
            user_id INT PRIMARY KEY,
            name VARCHAR(64) DEFAULT '影迷',
            bio TEXT,
            avatar_path TEXT
        )
    )");

    if (!ok) {
        qWarning() << "Create user_profile failed:" << q.lastError().text();
        return false;
    }

    return true;
}

int ServerDb::registerUser(const QString& username, const QString& passwordHash)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return 0;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("SELECT id FROM users WHERE username = :name");
    q.bindValue(":name", username);
    if (q.exec() && q.next()) return q.value(0).toInt();

    q.prepare("INSERT INTO users (username, password, created_at) VALUES (:name, :pwd, :time)");
    q.bindValue(":name", username);
    q.bindValue(":pwd", passwordHash);
    q.bindValue(":time", QDateTime::currentDateTime().toString(Qt::ISODate));
    if (q.exec()) return q.lastInsertId().toInt();
    return 0;
}

int ServerDb::loginUser(const QString& username, const QString& passwordHash)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return 0;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("SELECT id FROM users WHERE username = :name AND password = :pwd");
    q.bindValue(":name", username);
    q.bindValue(":pwd", passwordHash);
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

int ServerDb::getUserId(const QString& username)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return 0;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("SELECT id FROM users WHERE username = :name");
    q.bindValue(":name", username);
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

QString ServerDb::getUsername(int userId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return {};
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("SELECT username FROM users WHERE id = :id");
    q.bindValue(":id", userId);
    if (q.exec() && q.next()) return q.value(0).toString();
    return {};
}

bool ServerDb::addFriendRequest(int fromId, int toId)
{
    if (fromId == toId) return false;

    auto guard = m_pool->acquire();
    if (!guard.valid()) return false;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("SELECT id FROM friendships WHERE (from_id=:a AND to_id=:b) OR (from_id=:b2 AND to_id=:a2)");
    q.bindValue(":a", fromId);
    q.bindValue(":b", toId);
    q.bindValue(":a2", fromId);
    q.bindValue(":b2", toId);
    if (q.exec() && q.next()) return false;

    q.prepare("INSERT INTO friendships (from_id, to_id, status, created_at) VALUES (:a, :b, 'pending', :time)");
    q.bindValue(":a", fromId);
    q.bindValue(":b", toId);
    q.bindValue(":time", QDateTime::currentDateTime().toString(Qt::ISODate));
    return q.exec();
}

bool ServerDb::acceptFriend(int fromId, int toId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return false;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("UPDATE friendships SET status='accepted' WHERE from_id=:a AND to_id=:b AND status='pending'");
    q.bindValue(":a", fromId);
    q.bindValue(":b", toId);
    return q.exec() && q.numRowsAffected() > 0;
}

bool ServerDb::rejectFriend(int fromId, int toId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return false;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("DELETE FROM friendships WHERE from_id=:a AND to_id=:b AND status='pending'");
    q.bindValue(":a", fromId);
    q.bindValue(":b", toId);
    return q.exec();
}

QList<int> ServerDb::getFriendIds(int userId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return {};
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    QList<int> ids;
    q.prepare(R"(
        SELECT from_id, to_id FROM friendships
        WHERE (from_id=:uid OR to_id=:uid2) AND status='accepted'
    )");
    q.bindValue(":uid", userId);
    q.bindValue(":uid2", userId);
    q.exec();
    while (q.next()) {
        int a = q.value(0).toInt();
        int b = q.value(1).toInt();
        ids.append(a == userId ? b : a);
    }
    return ids;
}

QList<int> ServerDb::getPendingFromIds(int userId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return {};
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    QList<int> ids;
    q.prepare("SELECT from_id FROM friendships WHERE to_id=:uid AND status='pending'");
    q.bindValue(":uid", userId);
    q.exec();
    while (q.next())
        ids.append(q.value(0).toInt());
    return ids;
}

int ServerDb::getFriendshipStatus(int userId, int friendId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return 0;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare(R"(
        SELECT status FROM friendships
        WHERE (from_id=:a AND to_id=:b) OR (from_id=:b2 AND to_id=:a2)
    )");
    q.bindValue(":a", userId);
    q.bindValue(":b", friendId);
    q.bindValue(":a2", userId);
    q.bindValue(":b2", friendId);
    if (q.exec() && q.next()) {
        QString s = q.value(0).toString();
        if (s == "accepted") return 2;
        if (s == "pending") return 1;
    }
    return 0;
}

bool ServerDb::saveMessage(int fromId, int toId, const QString& content, const QString& time, int* outMsgId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return false;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("INSERT INTO messages (from_id, to_id, content, time, delivered) VALUES (:a, :b, :c, :t, 0)");
    q.bindValue(":a", fromId);
    q.bindValue(":b", toId);
    q.bindValue(":c", content);
    q.bindValue(":t", time);
    bool ok = q.exec();
    if (ok && outMsgId)
        *outMsgId = q.lastInsertId().toInt();
    return ok;
}

QList<ServerMsg> ServerDb::getOfflineMessages(int userId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return {};
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    QList<ServerMsg> msgs;
    q.prepare("SELECT id, from_id, to_id, content, time FROM messages WHERE to_id=:uid AND delivered=0 ORDER BY id");
    q.bindValue(":uid", userId);
    q.exec();
    while (q.next()) {
        ServerMsg m;
        m.id = q.value(0).toInt();
        m.fromId = q.value(1).toInt();
        m.toId = q.value(2).toInt();
        m.content = q.value(3).toString();
        m.time = q.value(4).toString();
        m.delivered = false;
        msgs.append(m);
    }
    return msgs;
}

QList<ServerMsg> ServerDb::getChatHistory(int userId, int friendId, int limit, int beforeMsgId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return {};
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    QList<ServerMsg> msgs;

    QString sql = R"(
        SELECT id, from_id, to_id, content, time FROM messages
        WHERE ((from_id=:me AND to_id=:friend) OR (from_id=:friend2 AND to_id=:me2))
    )";

    if (beforeMsgId > 0)
        sql += " AND id < :beforeId";

    sql += " ORDER BY id DESC LIMIT :lim";

    q.prepare(sql);
    q.bindValue(":me", userId);
    q.bindValue(":friend", friendId);
    q.bindValue(":me2", userId);
    q.bindValue(":friend2", friendId);
    if (beforeMsgId > 0)
        q.bindValue(":beforeId", beforeMsgId);
    q.bindValue(":lim", limit);

    if (!q.exec()) {
        qWarning() << "getChatHistory failed:" << q.lastError().text();
        return msgs;
    }

    while (q.next()) {
        ServerMsg m;
        m.id = q.value(0).toInt();
        m.fromId = q.value(1).toInt();
        m.toId = q.value(2).toInt();
        m.content = q.value(3).toString();
        m.time = q.value(4).toString();
        m.delivered = true;
        msgs.append(m);
    }

    std::reverse(msgs.begin(), msgs.end());
    return msgs;
}

void ServerDb::markDelivered(int userId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("UPDATE messages SET delivered=1 WHERE to_id=:uid AND delivered=0");
    q.bindValue(":uid", userId);
    q.exec();
}

// --- Reviews ---

bool ServerDb::saveReview(int userId, const QString& doubanId, const QString& movieName,
                           double rating, const QString& content, bool isWished, bool isWatched,
                           const QString& posterUrl)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return false;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);

    q.prepare(R"(
        INSERT INTO reviews (user_id, douban_id, movie_name, rating, content, is_wished, is_watched, poster_url, create_time, update_time)
        VALUES (:uid, :did, :mname, :rating, :content, :wished, :watched, :poster, :now, :now)
        ON DUPLICATE KEY UPDATE
            movie_name = VALUES(movie_name),
            rating = VALUES(rating),
            content = VALUES(content),
            is_wished = VALUES(is_wished),
            is_watched = VALUES(is_watched),
            poster_url = VALUES(poster_url),
            update_time = VALUES(update_time)
    )");
    q.bindValue(":uid", userId);
    q.bindValue(":did", doubanId);
    q.bindValue(":mname", movieName);
    q.bindValue(":rating", rating);
    q.bindValue(":content", content);
    q.bindValue(":wished", isWished ? 1 : 0);
    q.bindValue(":watched", isWatched ? 1 : 0);
    q.bindValue(":poster", posterUrl);
    q.bindValue(":now", now);

    if (!q.exec()) {
        qWarning() << "saveReview failed:" << q.lastError().text();
        return false;
    }
    return true;
}

ReviewData ServerDb::getReview(int userId, const QString& doubanId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return {};
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("SELECT id, douban_id, movie_name, rating, content, is_wished, is_watched, poster_url, create_time, update_time FROM reviews WHERE user_id=:uid AND douban_id=:did");
    q.bindValue(":uid", userId);
    q.bindValue(":did", doubanId);

    if (q.exec() && q.next()) {
        ReviewData r;
        r.id = q.value(0).toInt();
        r.userId = userId;
        r.doubanId = q.value(1).toString();
        r.movieName = q.value(2).toString();
        r.rating = q.value(3).toDouble();
        r.content = q.value(4).toString();
        r.isWished = q.value(5).toBool();
        r.isWatched = q.value(6).toBool();
        r.posterUrl = q.value(7).toString();
        r.createTime = q.value(8).toString();
        r.updateTime = q.value(9).toString();
        return r;
    }
    return {};
}

QList<ReviewData> ServerDb::getAllReviews(int userId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return {};
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    QList<ReviewData> list;
    q.prepare("SELECT id, douban_id, movie_name, rating, content, is_wished, is_watched, poster_url, create_time, update_time FROM reviews WHERE user_id=:uid ORDER BY update_time DESC");
    q.bindValue(":uid", userId);

    if (!q.exec()) return list;

    while (q.next()) {
        ReviewData r;
        r.id = q.value(0).toInt();
        r.userId = userId;
        r.doubanId = q.value(1).toString();
        r.movieName = q.value(2).toString();
        r.rating = q.value(3).toDouble();
        r.content = q.value(4).toString();
        r.isWished = q.value(5).toBool();
        r.isWatched = q.value(6).toBool();
        r.posterUrl = q.value(7).toString();
        r.createTime = q.value(8).toString();
        r.updateTime = q.value(9).toString();
        list.append(r);
    }
    return list;
}

bool ServerDb::deleteReview(int userId, const QString& doubanId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return false;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("DELETE FROM reviews WHERE user_id=:uid AND douban_id=:did");
    q.bindValue(":uid", userId);
    q.bindValue(":did", doubanId);
    return q.exec() && q.numRowsAffected() > 0;
}

QList<ReviewData> ServerDb::getWishList(int userId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return {};
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    QList<ReviewData> list;
    q.prepare("SELECT id, douban_id, movie_name, rating, poster_url, create_time FROM reviews WHERE user_id=:uid AND is_wished=1 ORDER BY create_time DESC");
    q.bindValue(":uid", userId);

    if (!q.exec()) return list;

    while (q.next()) {
        ReviewData r;
        r.id = q.value(0).toInt();
        r.userId = userId;
        r.doubanId = q.value(1).toString();
        r.movieName = q.value(2).toString();
        r.rating = q.value(3).toDouble();
        r.posterUrl = q.value(4).toString();
        r.createTime = q.value(5).toString();
        r.isWished = true;
        list.append(r);
    }
    return list;
}

QList<ReviewData> ServerDb::getWatchedList(int userId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return {};
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    QList<ReviewData> list;
    q.prepare("SELECT id, douban_id, movie_name, rating, poster_url, create_time FROM reviews WHERE user_id=:uid AND is_watched=1 ORDER BY create_time DESC");
    q.bindValue(":uid", userId);

    if (!q.exec()) return list;

    while (q.next()) {
        ReviewData r;
        r.id = q.value(0).toInt();
        r.userId = userId;
        r.doubanId = q.value(1).toString();
        r.movieName = q.value(2).toString();
        r.rating = q.value(3).toDouble();
        r.posterUrl = q.value(4).toString();
        r.createTime = q.value(5).toString();
        r.isWatched = true;
        list.append(r);
    }
    return list;
}

// --- Profile ---

ProfileData ServerDb::getProfile(int userId)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return {};
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare("SELECT name, bio, avatar_path FROM user_profile WHERE user_id=:uid");
    q.bindValue(":uid", userId);

    if (q.exec() && q.next()) {
        ProfileData p;
        p.name = q.value(0).toString();
        p.bio = q.value(1).toString();
        p.avatarPath = q.value(2).toString();
        return p;
    }

    // Auto-create default profile with username as default name
    QString defaultName = "影迷";
    q.prepare("SELECT username FROM users WHERE id=:uid");
    q.bindValue(":uid", userId);
    if (q.exec() && q.next())
        defaultName = q.value(0).toString();

    q.prepare("INSERT IGNORE INTO user_profile (user_id, name) VALUES (:uid, :name)");
    q.bindValue(":uid", userId);
    q.bindValue(":name", defaultName);
    q.exec();

    return ProfileData{defaultName, "", ""};
}

bool ServerDb::saveProfile(int userId, const QString& name, const QString& bio)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return false;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare(R"(
        INSERT INTO user_profile (user_id, name, bio) VALUES (:uid, :name, :bio)
        ON DUPLICATE KEY UPDATE name = VALUES(name), bio = VALUES(bio)
    )");
    q.bindValue(":uid", userId);
    q.bindValue(":name", name);
    q.bindValue(":bio", bio);
    return q.exec();
}

bool ServerDb::saveAvatarPath(int userId, const QString& avatarPath)
{
    auto guard = m_pool->acquire();
    if (!guard.valid()) return false;
    QSqlDatabase db = QSqlDatabase::database(guard.name());
    QSqlQuery q(db);

    q.prepare(R"(
        INSERT INTO user_profile (user_id, avatar_path) VALUES (:uid, :path)
        ON DUPLICATE KEY UPDATE avatar_path = VALUES(avatar_path)
    )");
    q.bindValue(":uid", userId);
    q.bindValue(":path", avatarPath);
    return q.exec();
}
