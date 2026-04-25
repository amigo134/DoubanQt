#include "serverdb.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QDebug>

ServerDb::ServerDb(QObject* parent)
    : QObject(parent)
{
}

bool ServerDb::initialize()
{
    QString dataPath = QCoreApplication::applicationDirPath();
    QString dbPath = dataPath + "/chatserver.db";
    qDebug() << "Server DB path:" << dbPath;

    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        m_db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    }
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Server DB open failed:" << m_db.lastError().text();
        return false;
    }

    return createTables();
}

bool ServerDb::createTables()
{
    QSqlQuery q(m_db);

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            created_at TEXT
        )
    )");

    bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS friendships (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_id INTEGER NOT NULL,
            to_id INTEGER NOT NULL,
            status TEXT DEFAULT 'pending',
            created_at TEXT,
            UNIQUE(from_id, to_id)
        )
    )");

    if (!ok) {
        qWarning() << "Create friendships failed:" << q.lastError().text();
        return false;
    }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_id INTEGER NOT NULL,
            to_id INTEGER NOT NULL,
            content TEXT,
            time TEXT,
            delivered INTEGER DEFAULT 0
        )
    )");

    if (!ok) {
        qWarning() << "Create messages failed:" << q.lastError().text();
        return false;
    }

    return true;
}

int ServerDb::registerUser(const QString& username, const QString& passwordHash)
{
    QSqlQuery q(m_db);
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
    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM users WHERE username = :name AND password = :pwd");
    q.bindValue(":name", username);
    q.bindValue(":pwd", passwordHash);
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

int ServerDb::getUserId(const QString& username)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM users WHERE username = :name");
    q.bindValue(":name", username);
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

QString ServerDb::getUsername(int userId)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT username FROM users WHERE id = :id");
    q.bindValue(":id", userId);
    if (q.exec() && q.next()) return q.value(0).toString();
    return QString();
}

bool ServerDb::addFriendRequest(int fromId, int toId)
{
    if (fromId == toId) return false;

    QSqlQuery q(m_db);
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
    QSqlQuery q(m_db);
    q.prepare("UPDATE friendships SET status='accepted' WHERE from_id=:a AND to_id=:b AND status='pending'");
    q.bindValue(":a", fromId);
    q.bindValue(":b", toId);
    return q.exec() && q.numRowsAffected() > 0;
}

bool ServerDb::rejectFriend(int fromId, int toId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM friendships WHERE from_id=:a AND to_id=:b AND status='pending'");
    q.bindValue(":a", fromId);
    q.bindValue(":b", toId);
    return q.exec();
}

QList<int> ServerDb::getFriendIds(int userId)
{
    QList<int> ids;
    QSqlQuery q(m_db);
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
    QList<int> ids;
    QSqlQuery q(m_db);
    q.prepare("SELECT from_id FROM friendships WHERE to_id=:uid AND status='pending'");
    q.bindValue(":uid", userId);
    q.exec();
    while (q.next()) {
        ids.append(q.value(0).toInt());
    }
    return ids;
}

int ServerDb::getFriendshipStatus(int userId, int friendId)
{
    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT status FROM friendships
        WHERE (from_id=:a AND to_id=:b) OR (from_id=:b2 AND to_id=:a2)
    ))");
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
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO messages (from_id, to_id, content, time, delivered) VALUES (:a, :b, :c, :t, 0)");
    q.bindValue(":a", fromId);
    q.bindValue(":b", toId);
    q.bindValue(":c", content);
    q.bindValue(":t", time);
    bool ok = q.exec();
    if (ok && outMsgId) {
        *outMsgId = q.lastInsertId().toInt();
    }
    return ok;
}

QList<ServerMsg> ServerDb::getOfflineMessages(int userId)
{
    QList<ServerMsg> msgs;
    QSqlQuery q(m_db);
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
    QList<ServerMsg> msgs;
    QSqlQuery q(m_db);

    QString sql = R"(
        SELECT id, from_id, to_id, content, time FROM messages
        WHERE ((from_id=:me AND to_id=:friend) OR (from_id=:friend2 AND to_id=:me2))
    )";

    if (beforeMsgId > 0) {
        sql += " AND id < :beforeId";
    }

    sql += " ORDER BY id DESC LIMIT :lim";

    q.prepare(sql);
    q.bindValue(":me", userId);
    q.bindValue(":friend", friendId);
    q.bindValue(":me2", userId);
    q.bindValue(":friend2", friendId);
    if (beforeMsgId > 0) {
        q.bindValue(":beforeId", beforeMsgId);
    }
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
    QSqlQuery q(m_db);
    q.prepare("UPDATE messages SET delivered=1 WHERE to_id=:uid AND delivered=0");
    q.bindValue(":uid", userId);
    q.exec();
}
