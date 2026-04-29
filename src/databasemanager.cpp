#include "databasemanager.h"
#include "chatmanager.h"
#include "serverapiclient.h"
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

void DatabaseManager::setServerApiClient(ServerApiClient* api)
{
    m_serverApi = api;
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

int DatabaseManager::serverUserId() const
{
    return m_chatMgr ? m_chatMgr->serverUserId() : 0;
}

bool DatabaseManager::saveReview(const UserReview& review)
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return false;

    bool result = false;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::reviewSaved,
        [&](bool success, const QString& did) {
            if (did == review.doubanId) { result = success; loop.quit(); }
        });

    m_serverApi->saveReview(uid, review.doubanId, review.movieName, review.rating,
                            review.content, review.isWished, review.isWatched, review.posterUrl);
    loop.exec();
    disconnect(conn);
    return result;
}

UserReview DatabaseManager::getReview(const QString& doubanId)
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return {};

    UserReview result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn1 = connect(m_serverApi, &ServerApiClient::reviewReceived,
        [&](const UserReview& review) {
            if (review.doubanId == doubanId) { result = review; loop.quit(); }
        });
    QMetaObject::Connection conn2 = connect(m_serverApi, &ServerApiClient::reviewSaved,
        [&](bool success, const QString& did) {
            if (did == doubanId && !success) loop.quit();
        });

    m_serverApi->getReview(uid, doubanId);
    loop.exec();
    disconnect(conn1);
    disconnect(conn2);
    return result;
}

QList<UserReview> DatabaseManager::getAllReviews()
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return {};

    QList<UserReview> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::reviewsListReceived,
        [&](const QList<UserReview>& list) { result = list; loop.quit(); });

    m_serverApi->getAllReviews(uid);
    loop.exec();
    disconnect(conn);
    return result;
}

QString DatabaseManager::getProfileName()
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return "影迷";

    QString result = "影迷";
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::profileReceived,
        [&](const QString& name, const QString&, const QString&) {
            result = name; loop.quit();
        });

    m_serverApi->getProfile(uid);
    loop.exec();
    disconnect(conn);
    return result;
}

QString DatabaseManager::getProfileBio()
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return {};

    QString result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::profileReceived,
        [&](const QString&, const QString& bio, const QString&) {
            result = bio; loop.quit();
        });

    m_serverApi->getProfile(uid);
    loop.exec();
    disconnect(conn);
    return result;
}

void DatabaseManager::saveProfile(const QString& name, const QString& bio)
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return;

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::profileSaved,
        [&](bool) { loop.quit(); });

    m_serverApi->saveProfile(uid, name, bio);
    loop.exec();
    disconnect(conn);
}

QString DatabaseManager::getAvatarPath()
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return {};

    QString result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::profileReceived,
        [&](const QString&, const QString&, const QString& path) {
            result = path; loop.quit();
        });

    m_serverApi->getProfile(uid);
    loop.exec();
    disconnect(conn);
    return result;
}

void DatabaseManager::saveAvatarPath(const QString& path)
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return;

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::avatarSaved,
        [&](bool) { loop.quit(); });

    m_serverApi->uploadAvatar(uid, path);
    loop.exec();
    disconnect(conn);
}

bool DatabaseManager::deleteReview(const QString& doubanId)
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return false;

    bool result = false;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::reviewDeleted,
        [&](bool success, const QString& did) {
            if (did == doubanId) { result = success; loop.quit(); }
        });

    m_serverApi->deleteReview(uid, doubanId);
    loop.exec();
    disconnect(conn);
    return result;
}

void DatabaseManager::updatePosterUrl(const QString& doubanId, const QString& posterUrl)
{
    if (posterUrl.isEmpty()) return;
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return;
    // Fire-and-forget: get current review, merge, re-save
    UserReview existing = getReview(doubanId);
    existing.posterUrl = posterUrl;
    saveReview(existing);
}

bool DatabaseManager::setWished(const QString& doubanId, const QString& movieName, bool wished, const QString& posterUrl)
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return false;

    bool result = false;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::reviewSaved,
        [&](bool success, const QString& did) {
            if (did == doubanId) { result = success; loop.quit(); }
        });

    m_serverApi->saveReview(uid, doubanId, movieName, 0, QString(), wished, false, posterUrl);
    loop.exec();
    disconnect(conn);
    return result;
}

bool DatabaseManager::setWatched(const QString& doubanId, const QString& movieName, bool watched, const QString& posterUrl)
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return false;

    bool result = false;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::reviewSaved,
        [&](bool success, const QString& did) {
            if (did == doubanId) { result = success; loop.quit(); }
        });

    m_serverApi->saveReview(uid, doubanId, movieName, 0, QString(), false, watched, posterUrl);
    loop.exec();
    disconnect(conn);
    return result;
}

QList<UserReview> DatabaseManager::getWishList()
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return {};

    QList<UserReview> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::wishListReceived,
        [&](const QList<UserReview>& list) { result = list; loop.quit(); });

    m_serverApi->getWishList(uid);
    loop.exec();
    disconnect(conn);
    return result;
}

QList<UserReview> DatabaseManager::getWatchedList()
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return {};

    QList<UserReview> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::watchedListReceived,
        [&](const QList<UserReview>& list) { result = list; loop.quit(); });

    m_serverApi->getWatchedList(uid);
    loop.exec();
    disconnect(conn);
    return result;
}

QList<UserReview> DatabaseManager::getMovieReviews(const QString& doubanId)
{
    if (!m_serverApi) return {};

    QList<UserReview> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::movieReviewsReceived,
        [&](const QString& did, const QList<UserReview>& list) {
            if (did == doubanId) { result = list; loop.quit(); }
        });

    m_serverApi->getMovieReviews(doubanId);
    loop.exec();
    disconnect(conn);
    return result;
}

QList<UserReview> DatabaseManager::getUserReviews(const QString& username)
{
    int uid = serverUserId();
    if (!m_serverApi || uid == 0) return {};

    QList<UserReview> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    QMetaObject::Connection conn = connect(m_serverApi, &ServerApiClient::userReviewsReceived,
        [&](const QString&, const QList<UserReview>& list) { result = list; loop.quit(); });

    m_serverApi->getUserReviews(uid);
    loop.exec();
    disconnect(conn);
    return result;
}
