#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QCryptographicHash>
#include <QCoreApplication>

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::initialize()
{
    QString dataPath = QCoreApplication::applicationDirPath();
    QDir().mkpath(dataPath);
    QString dbPath = dataPath + "/doubanqt.db";
    qDebug() << "Database path:" << dbPath;

    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        m_db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    }
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "无法打开数据库:" << m_db.lastError().text();
        m_ready = false;
        return false;
    }

    m_ready = createTables();
    return m_ready;
}

bool DatabaseManager::isReady() const
{
    return m_ready && m_db.isOpen();
}

bool DatabaseManager::hasUsers()
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.exec("SELECT COUNT(*) FROM users");
    if (query.next()) return query.value(0).toInt() > 0;
    return false;
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            created_at TEXT
        )
    )");

    bool ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS reviews (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER DEFAULT 1,
            douban_id TEXT NOT NULL,
            movie_name TEXT,
            rating REAL DEFAULT 0,
            content TEXT,
            is_wished INTEGER DEFAULT 0,
            is_watched INTEGER DEFAULT 0,
            poster_url TEXT,
            create_time TEXT,
            update_time TEXT,
            UNIQUE(user_id, douban_id)
        )
    )");

    if (!ok) {
        qWarning() << "创建表失败:" << query.lastError().text();
        return false;
    }

    query.exec("ALTER TABLE reviews ADD COLUMN user_id INTEGER DEFAULT 1");
    query.exec("ALTER TABLE reviews ADD COLUMN poster_url TEXT");

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS user_profile (
            user_id INTEGER PRIMARY KEY,
            name TEXT DEFAULT '影迷',
            bio TEXT DEFAULT '记录每一部看过的电影',
            avatar_path TEXT
        )
    )");

    return true;
}

static QString hashPassword(const QString& password)
{
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

int DatabaseManager::registerUser(const QString& username, const QString& password)
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM users WHERE username = :name");
    query.bindValue(":name", username);
    if (query.exec() && query.next()) return 0;

    query.prepare("INSERT INTO users (username, password, created_at) VALUES (:name, :pwd, :time)");
    query.bindValue(":name", username);
    query.bindValue(":pwd", hashPassword(password));
    query.bindValue(":time", QDateTime::currentDateTime().toString(Qt::ISODate));
    if (query.exec()) return query.lastInsertId().toInt();
    return 0;
}

int DatabaseManager::loginUser(const QString& username, const QString& password)
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM users WHERE username = :name AND password = :pwd");
    query.bindValue(":name", username);
    query.bindValue(":pwd", hashPassword(password));
    if (query.exec() && query.next()) return query.value(0).toInt();
    return 0;
}

void DatabaseManager::setCurrentUser(int userId)
{
    m_currentUserId = userId;
    ensureProfile();
}

int DatabaseManager::currentUserId() const
{
    return m_currentUserId;
}

QString DatabaseManager::currentUsername()
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("SELECT username FROM users WHERE id = :id");
    query.bindValue(":id", m_currentUserId);
    if (query.exec() && query.next())
        return query.value(0).toString();
    return QString();
}

void DatabaseManager::ensureProfile()
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery check(m_db);
    check.prepare("SELECT user_id FROM user_profile WHERE user_id = :id");
    check.bindValue(":id", m_currentUserId);
    if (check.exec() && check.next()) return;
    QString defaultName = currentUsername();
    if (defaultName.isEmpty()) defaultName = "影迷";
    QSqlQuery ins(m_db);
    ins.prepare("INSERT INTO user_profile (user_id, name, bio, avatar_path) VALUES (:id, :name, '记录每一部看过的电影', NULL)");
    ins.bindValue(":id", m_currentUserId);
    ins.bindValue(":name", defaultName);
    if (!ins.exec()) {
        qWarning() << "ensureProfile insert failed:" << ins.lastError().text();
    }
}

bool DatabaseManager::saveReview(const UserReview& review)
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO reviews (user_id, douban_id, movie_name, rating, content, is_wished, is_watched, poster_url, create_time, update_time)
        VALUES (:user_id, :douban_id, :movie_name, :rating, :content, :is_wished, :is_watched, :poster_url, :create_time, :update_time)
        ON CONFLICT(user_id, douban_id) DO UPDATE SET
            movie_name = :movie_name2,
            rating = :rating2,
            content = :content2,
            is_wished = :is_wished2,
            is_watched = :is_watched2,
            poster_url = :poster_url2,
            update_time = :update_time2
    )");

    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    query.bindValue(":user_id", m_currentUserId);
    query.bindValue(":douban_id", review.doubanId);
    query.bindValue(":movie_name", review.movieName);
    query.bindValue(":rating", review.rating);
    query.bindValue(":content", review.content);
    query.bindValue(":is_wished", review.isWished ? 1 : 0);
    query.bindValue(":is_watched", review.isWatched ? 1 : 0);
    query.bindValue(":poster_url", review.posterUrl);
    query.bindValue(":create_time", review.createTime.isEmpty() ? now : review.createTime);
    query.bindValue(":update_time", now);
    query.bindValue(":movie_name2", review.movieName);
    query.bindValue(":rating2", review.rating);
    query.bindValue(":content2", review.content);
    query.bindValue(":is_wished2", review.isWished ? 1 : 0);
    query.bindValue(":is_watched2", review.isWatched ? 1 : 0);
    query.bindValue(":poster_url2", review.posterUrl);
    query.bindValue(":update_time2", now);

    if (!query.exec()) {
        qWarning() << "保存评价失败:" << query.lastError().text();
        return false;
    }
    return true;
}

UserReview DatabaseManager::getReview(const QString& doubanId)
{
    UserReview review;
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM reviews WHERE user_id = :uid AND douban_id = :id");
    query.bindValue(":uid", m_currentUserId);
    query.bindValue(":id", doubanId);

    if (query.exec() && query.next()) {
        review.id = query.value("id").toInt();
        review.doubanId = query.value("douban_id").toString();
        review.movieName = query.value("movie_name").toString();
        review.rating = query.value("rating").toDouble();
        review.content = query.value("content").toString();
        review.isWished = query.value("is_wished").toInt() != 0;
        review.isWatched = query.value("is_watched").toInt() != 0;
        review.posterUrl = query.value("poster_url").toString();
        review.createTime = query.value("create_time").toString();
    }
    return review;
}

QList<UserReview> DatabaseManager::getAllReviews()
{
    QList<UserReview> reviews;
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM reviews WHERE user_id = :uid ORDER BY update_time DESC");
    query.bindValue(":uid", m_currentUserId);
    query.exec();
    while (query.next()) {
        UserReview review;
        review.id = query.value("id").toInt();
        review.doubanId = query.value("douban_id").toString();
        review.movieName = query.value("movie_name").toString();
        review.rating = query.value("rating").toDouble();
        review.content = query.value("content").toString();
        review.isWished = query.value("is_wished").toInt() != 0;
        review.isWatched = query.value("is_watched").toInt() != 0;
        review.posterUrl = query.value("poster_url").toString();
        review.createTime = query.value("create_time").toString();
        reviews.append(review);
    }
    return reviews;
}

QString DatabaseManager::getProfileName()
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("SELECT name FROM user_profile WHERE user_id = :id");
    query.bindValue(":id", m_currentUserId);
    if (query.exec() && query.next())
        return query.value(0).toString();
    return "影迷";
}

QString DatabaseManager::getProfileBio()
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("SELECT bio FROM user_profile WHERE user_id = :id");
    query.bindValue(":id", m_currentUserId);
    if (query.exec() && query.next())
        return query.value(0).toString();
    return "记录每一部看过的电影";
}

void DatabaseManager::saveProfile(const QString& name, const QString& bio)
{
    if (!m_db.isOpen()) m_db.open();
    ensureProfile();
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO user_profile (user_id, name, bio, avatar_path) VALUES (:id, :name, :bio, (SELECT avatar_path FROM user_profile WHERE user_id = :id2))");
    query.bindValue(":id", m_currentUserId);
    query.bindValue(":name", name);
    query.bindValue(":bio", bio);
    query.bindValue(":id2", m_currentUserId);
    query.exec();
}

QString DatabaseManager::getAvatarPath()
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("SELECT avatar_path FROM user_profile WHERE user_id = :id");
    query.bindValue(":id", m_currentUserId);
    if (query.exec() && query.next())
        return query.value(0).toString();
    return QString();
}

void DatabaseManager::saveAvatarPath(const QString& path)
{
    if (!m_db.isOpen()) m_db.open();
    ensureProfile();
    QSqlQuery query(m_db);
    query.prepare("UPDATE user_profile SET avatar_path = :path WHERE user_id = :id");
    query.bindValue(":path", path);
    query.bindValue(":id", m_currentUserId);
    query.exec();
}

bool DatabaseManager::deleteReview(const QString& doubanId)
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM reviews WHERE user_id = :uid AND douban_id = :id");
    query.bindValue(":uid", m_currentUserId);
    query.bindValue(":id", doubanId);
    return query.exec();
}

void DatabaseManager::updatePosterUrl(const QString& doubanId, const QString& posterUrl)
{
    if (posterUrl.isEmpty()) return;
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("UPDATE reviews SET poster_url = :poster WHERE user_id = :uid AND douban_id = :id AND (poster_url IS NULL OR poster_url = '')");
    query.bindValue(":poster", posterUrl);
    query.bindValue(":uid", m_currentUserId);
    query.bindValue(":id", doubanId);
    query.exec();
}

bool DatabaseManager::setWished(const QString& doubanId, const QString& movieName, bool wished, const QString& posterUrl)
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO reviews (user_id, douban_id, movie_name, is_wished, poster_url, create_time, update_time)
        VALUES (:uid, :id, :name, :wished, :poster, :time, :time2)
        ON CONFLICT(user_id, douban_id) DO UPDATE SET is_wished = :wished2, poster_url = :poster2, update_time = :time3
    )");
    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    query.bindValue(":uid", m_currentUserId);
    query.bindValue(":id", doubanId);
    query.bindValue(":name", movieName);
    query.bindValue(":wished", wished ? 1 : 0);
    query.bindValue(":poster", posterUrl);
    query.bindValue(":time", now);
    query.bindValue(":time2", now);
    query.bindValue(":wished2", wished ? 1 : 0);
    query.bindValue(":poster2", posterUrl);
    query.bindValue(":time3", now);
    return query.exec();
}

bool DatabaseManager::setWatched(const QString& doubanId, const QString& movieName, bool watched, const QString& posterUrl)
{
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO reviews (user_id, douban_id, movie_name, is_watched, poster_url, create_time, update_time)
        VALUES (:uid, :id, :name, :watched, :poster, :time, :time2)
        ON CONFLICT(user_id, douban_id) DO UPDATE SET is_watched = :watched2, poster_url = :poster2, update_time = :time3
    )");
    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    query.bindValue(":uid", m_currentUserId);
    query.bindValue(":id", doubanId);
    query.bindValue(":name", movieName);
    query.bindValue(":watched", watched ? 1 : 0);
    query.bindValue(":poster", posterUrl);
    query.bindValue(":time", now);
    query.bindValue(":time2", now);
    query.bindValue(":watched2", watched ? 1 : 0);
    query.bindValue(":poster2", posterUrl);
    query.bindValue(":time3", now);
    return query.exec();
}

QList<UserReview> DatabaseManager::getWishList()
{
    QList<UserReview> reviews;
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM reviews WHERE user_id = :uid AND is_wished = 1 ORDER BY update_time DESC");
    query.bindValue(":uid", m_currentUserId);
    query.exec();
    while (query.next()) {
        UserReview review;
        review.doubanId = query.value("douban_id").toString();
        review.movieName = query.value("movie_name").toString();
        review.rating = query.value("rating").toDouble();
        review.isWished = true;
        review.posterUrl = query.value("poster_url").toString();
        review.createTime = query.value("create_time").toString();
        reviews.append(review);
    }
    return reviews;
}

QList<UserReview> DatabaseManager::getWatchedList()
{
    QList<UserReview> reviews;
    if (!m_db.isOpen()) m_db.open();
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM reviews WHERE user_id = :uid AND is_watched = 1 ORDER BY update_time DESC");
    query.bindValue(":uid", m_currentUserId);
    query.exec();
    while (query.next()) {
        UserReview review;
        review.doubanId = query.value("douban_id").toString();
        review.movieName = query.value("movie_name").toString();
        review.rating = query.value("rating").toDouble();
        review.isWatched = true;
        review.posterUrl = query.value("poster_url").toString();
        review.createTime = query.value("create_time").toString();
        reviews.append(review);
    }
    return reviews;
}
