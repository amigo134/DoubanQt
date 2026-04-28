#include "databasemanager.h"
#include "chatmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>

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
}

void DatabaseManager::setChatManager(ChatManager* mgr)
{
    m_chatMgr = mgr;
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
    // Server auto-creates profile on first getProfile(); no local action needed
}

bool DatabaseManager::saveReview(const UserReview& review)
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return false;

    bool result = false;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::reviewSaved,
        [&](bool success, const QString& did) {
            if (did == review.doubanId) { result = success; loop.quit(); }
        });

    m_chatMgr->requestSaveReview(review.doubanId, review.movieName, review.rating,
                                  review.content, review.isWished, review.isWatched, review.posterUrl);
    loop.exec();
    disconnect(conn);
    return result;
}

UserReview DatabaseManager::getReview(const QString& doubanId)
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return {};

    UserReview result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn1 = connect(m_chatMgr, &ChatManager::reviewReceived,
        [&](const UserReview& review) {
            if (review.doubanId == doubanId) { result = review; loop.quit(); }
        });
    QMetaObject::Connection conn2 = connect(m_chatMgr, &ChatManager::reviewSaved,
        [&](bool success, const QString& did) {
            if (did == doubanId && !success) loop.quit();
        });

    m_chatMgr->requestGetReview(doubanId);
    loop.exec();
    disconnect(conn1);
    disconnect(conn2);
    return result;
}

QList<UserReview> DatabaseManager::getAllReviews()
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return {};

    QList<UserReview> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::reviewsListReceived,
        [&](const QList<UserReview>& list) { result = list; loop.quit(); });

    m_chatMgr->requestGetAllReviews();
    loop.exec();
    disconnect(conn);
    return result;
}

QString DatabaseManager::getProfileName()
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return "影迷";

    QString result = "影迷";
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::profileReceived,
        [&](const QString& name, const QString&, const QString&) {
            result = name; loop.quit();
        });

    m_chatMgr->requestGetProfile();
    loop.exec();
    disconnect(conn);
    return result;
}

QString DatabaseManager::getProfileBio()
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return {};

    QString result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::profileReceived,
        [&](const QString&, const QString& bio, const QString&) {
            result = bio; loop.quit();
        });

    m_chatMgr->requestGetProfile();
    loop.exec();
    disconnect(conn);
    return result;
}

void DatabaseManager::saveProfile(const QString& name, const QString& bio)
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return;

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::profileSaved,
        [&](bool) { loop.quit(); });

    m_chatMgr->requestSaveProfile(name, bio);
    loop.exec();
    disconnect(conn);
}

QString DatabaseManager::getAvatarPath()
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return {};

    QString result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::profileReceived,
        [&](const QString&, const QString&, const QString& path) {
            result = path; loop.quit();
        });

    m_chatMgr->requestGetProfile();
    loop.exec();
    disconnect(conn);
    return result;
}

void DatabaseManager::saveAvatarPath(const QString& path)
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return;

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::avatarSaved,
        [&](bool) { loop.quit(); });

    m_chatMgr->requestSaveAvatar(path);
    loop.exec();
    disconnect(conn);
}

bool DatabaseManager::deleteReview(const QString& doubanId)
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return false;

    bool result = false;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::reviewDeleted,
        [&](bool success, const QString& did) {
            if (did == doubanId) { result = success; loop.quit(); }
        });

    m_chatMgr->requestDeleteReview(doubanId);
    loop.exec();
    disconnect(conn);
    return result;
}

void DatabaseManager::updatePosterUrl(const QString& doubanId, const QString& posterUrl)
{
    if (posterUrl.isEmpty()) return;
    if (!m_chatMgr || !m_chatMgr->isConnected()) return;
    // Fire-and-forget: get current review, merge, re-save
    UserReview existing = getReview(doubanId);
    existing.posterUrl = posterUrl;
    saveReview(existing);
}

bool DatabaseManager::setWished(const QString& doubanId, const QString& movieName, bool wished, const QString& posterUrl)
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return false;

    bool result = false;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::reviewSaved,
        [&](bool success, const QString& did) {
            if (did == doubanId) { result = success; loop.quit(); }
        });

    m_chatMgr->requestSaveReview(doubanId, movieName, 0, QString(), wished, false, posterUrl);
    loop.exec();
    disconnect(conn);
    return result;
}

bool DatabaseManager::setWatched(const QString& doubanId, const QString& movieName, bool watched, const QString& posterUrl)
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return false;

    bool result = false;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::reviewSaved,
        [&](bool success, const QString& did) {
            if (did == doubanId) { result = success; loop.quit(); }
        });

    m_chatMgr->requestSaveReview(doubanId, movieName, 0, QString(), false, watched, posterUrl);
    loop.exec();
    disconnect(conn);
    return result;
}

QList<UserReview> DatabaseManager::getWishList()
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return {};

    QList<UserReview> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::wishListReceived,
        [&](const QList<UserReview>& list) { result = list; loop.quit(); });

    m_chatMgr->requestGetWishList();
    loop.exec();
    disconnect(conn);
    return result;
}

QList<UserReview> DatabaseManager::getWatchedList()
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return {};

    QList<UserReview> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::watchedListReceived,
        [&](const QList<UserReview>& list) { result = list; loop.quit(); });

    m_chatMgr->requestGetWatchedList();
    loop.exec();
    disconnect(conn);
    return result;
}

QList<UserReview> DatabaseManager::getMovieReviews(const QString& doubanId)
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return {};

    QList<UserReview> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::movieReviewsReceived,
        [&](const QString& did, const QList<UserReview>& list) {
            if (did == doubanId) { result = list; loop.quit(); }
        });

    m_chatMgr->requestMovieReviews(doubanId);
    loop.exec();
    disconnect(conn);
    return result;
}

QList<UserReview> DatabaseManager::getUserReviews(const QString& username)
{
    if (!m_chatMgr || !m_chatMgr->isConnected()) return {};

    QList<UserReview> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_chatMgr, &ChatManager::userReviewsReceived,
        [&](const QString& uname, const QList<UserReview>& list) {
            if (uname == username) { result = list; loop.quit(); }
        });

    m_chatMgr->requestUserReviews(username);
    loop.exec();
    disconnect(conn);
    return result;
}
