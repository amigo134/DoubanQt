#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QDateTime>

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
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    QString dbPath = dataPath + "/doubanqt.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "无法打开数据库:" << m_db.lastError().text();
        return false;
    }

    return createTables();
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);
    bool ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS reviews (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            douban_id TEXT UNIQUE NOT NULL,
            movie_name TEXT,
            rating REAL DEFAULT 0,
            content TEXT,
            is_wished INTEGER DEFAULT 0,
            is_watched INTEGER DEFAULT 0,
            create_time TEXT,
            update_time TEXT
        )
    )");

    if (!ok) {
        qWarning() << "创建表失败:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::saveReview(const UserReview& review)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO reviews (douban_id, movie_name, rating, content, is_wished, is_watched, create_time, update_time)
        VALUES (:douban_id, :movie_name, :rating, :content, :is_wished, :is_watched, :create_time, :update_time)
        ON CONFLICT(douban_id) DO UPDATE SET
            movie_name = :movie_name2,
            rating = :rating2,
            content = :content2,
            is_wished = :is_wished2,
            is_watched = :is_watched2,
            update_time = :update_time2
    )");

    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    query.bindValue(":douban_id", review.doubanId);
    query.bindValue(":movie_name", review.movieName);
    query.bindValue(":rating", review.rating);
    query.bindValue(":content", review.content);
    query.bindValue(":is_wished", review.isWished ? 1 : 0);
    query.bindValue(":is_watched", review.isWatched ? 1 : 0);
    query.bindValue(":create_time", review.createTime.isEmpty() ? now : review.createTime);
    query.bindValue(":update_time", now);
    query.bindValue(":movie_name2", review.movieName);
    query.bindValue(":rating2", review.rating);
    query.bindValue(":content2", review.content);
    query.bindValue(":is_wished2", review.isWished ? 1 : 0);
    query.bindValue(":is_watched2", review.isWatched ? 1 : 0);
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
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM reviews WHERE douban_id = :id");
    query.bindValue(":id", doubanId);

    if (query.exec() && query.next()) {
        review.id = query.value("id").toInt();
        review.doubanId = query.value("douban_id").toString();
        review.movieName = query.value("movie_name").toString();
        review.rating = query.value("rating").toDouble();
        review.content = query.value("content").toString();
        review.isWished = query.value("is_wished").toInt() != 0;
        review.isWatched = query.value("is_watched").toInt() != 0;
        review.createTime = query.value("create_time").toString();
    }
    return review;
}

QList<UserReview> DatabaseManager::getAllReviews()
{
    QList<UserReview> reviews;
    QSqlQuery query("SELECT * FROM reviews ORDER BY update_time DESC", m_db);

    while (query.next()) {
        UserReview review;
        review.id = query.value("id").toInt();
        review.doubanId = query.value("douban_id").toString();
        review.movieName = query.value("movie_name").toString();
        review.rating = query.value("rating").toDouble();
        review.content = query.value("content").toString();
        review.isWished = query.value("is_wished").toInt() != 0;
        review.isWatched = query.value("is_watched").toInt() != 0;
        review.createTime = query.value("create_time").toString();
        reviews.append(review);
    }
    return reviews;
}

bool DatabaseManager::deleteReview(const QString& doubanId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM reviews WHERE douban_id = :id");
    query.bindValue(":id", doubanId);
    return query.exec();
}

bool DatabaseManager::setWished(const QString& doubanId, const QString& movieName, bool wished)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO reviews (douban_id, movie_name, is_wished, create_time, update_time)
        VALUES (:id, :name, :wished, :time, :time2)
        ON CONFLICT(douban_id) DO UPDATE SET is_wished = :wished2, update_time = :time3
    )");
    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    query.bindValue(":id", doubanId);
    query.bindValue(":name", movieName);
    query.bindValue(":wished", wished ? 1 : 0);
    query.bindValue(":time", now);
    query.bindValue(":time2", now);
    query.bindValue(":wished2", wished ? 1 : 0);
    query.bindValue(":time3", now);
    return query.exec();
}

bool DatabaseManager::setWatched(const QString& doubanId, const QString& movieName, bool watched)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO reviews (douban_id, movie_name, is_watched, create_time, update_time)
        VALUES (:id, :name, :watched, :time, :time2)
        ON CONFLICT(douban_id) DO UPDATE SET is_watched = :watched2, update_time = :time3
    )");
    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    query.bindValue(":id", doubanId);
    query.bindValue(":name", movieName);
    query.bindValue(":watched", watched ? 1 : 0);
    query.bindValue(":time", now);
    query.bindValue(":time2", now);
    query.bindValue(":watched2", watched ? 1 : 0);
    query.bindValue(":time3", now);
    return query.exec();
}

QList<UserReview> DatabaseManager::getWishList()
{
    QList<UserReview> reviews;
    QSqlQuery query("SELECT * FROM reviews WHERE is_wished = 1 ORDER BY update_time DESC", m_db);
    while (query.next()) {
        UserReview review;
        review.doubanId = query.value("douban_id").toString();
        review.movieName = query.value("movie_name").toString();
        review.rating = query.value("rating").toDouble();
        review.isWished = true;
        review.createTime = query.value("create_time").toString();
        reviews.append(review);
    }
    return reviews;
}

QList<UserReview> DatabaseManager::getWatchedList()
{
    QList<UserReview> reviews;
    QSqlQuery query("SELECT * FROM reviews WHERE is_watched = 1 ORDER BY update_time DESC", m_db);
    while (query.next()) {
        UserReview review;
        review.doubanId = query.value("douban_id").toString();
        review.movieName = query.value("movie_name").toString();
        review.rating = query.value("rating").toDouble();
        review.isWatched = true;
        review.createTime = query.value("create_time").toString();
        reviews.append(review);
    }
    return reviews;
}
