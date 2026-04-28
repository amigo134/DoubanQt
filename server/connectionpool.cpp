#include "connectionpool.h"
#include <QSqlError>
#include <QDebug>

// --- DbGuard ---

DbGuard::DbGuard() = default;

DbGuard::DbGuard(const QString& name, ConnectionPool* pool)
    : m_name(name), m_pool(pool) {}

DbGuard::~DbGuard()
{
    if (!m_name.isEmpty() && m_pool)
        m_pool->release(m_name);
}

DbGuard::DbGuard(DbGuard&& other)
    : m_name(std::move(other.m_name)), m_pool(other.m_pool)
{
    other.m_pool = nullptr;
    other.m_name.clear();
}

DbGuard& DbGuard::operator=(DbGuard&& other)
{
    if (this != &other) {
        if (!m_name.isEmpty() && m_pool)
            m_pool->release(m_name);
        m_name = std::move(other.m_name);
        m_pool = other.m_pool;
        other.m_pool = nullptr;
        other.m_name.clear();
    }
    return *this;
}

// --- ConnectionPool ---

ConnectionPool::ConnectionPool(int poolSize, QObject* parent)
    : QObject(parent), m_poolSize(poolSize) {}

ConnectionPool::~ConnectionPool()
{
    QMutexLocker locker(&m_mutex);
    for (auto it = m_threadPools.begin(); it != m_threadPools.end(); ++it) {
        QSet<QString> all = it->available + it->busy;
        for (const QString& name : all)
            QSqlDatabase::removeDatabase(name);
    }
}

bool ConnectionPool::initialize(const QString& host, int port,
                                 const QString& user, const QString& password,
                                 const QString& dbName)
{
    m_host = host;
    m_port = port;
    m_user = user;
    m_password = password;
    m_dbName = dbName;

    // Pre-create connections for the calling thread (main thread)
    QMutexLocker locker(&m_mutex);
    Qt::HANDLE tid = QThread::currentThreadId();
    PerThreadPool& ptp = m_threadPools[tid];
    locker.unlock();

    createConnectionsForThread(ptp);

    qDebug() << "Connection pool ready," << m_poolSize << "connections per thread";
    return true;
}

void ConnectionPool::createConnectionsForThread(PerThreadPool& ptp)
{
    for (int i = 0; i < m_poolSize; ++i) {
        QString connName = QString("chat_pool_%1_%2")
                               .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16)
                               .arg(i);
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", connName);
        db.setHostName(m_host);
        db.setPort(m_port);
        db.setUserName(m_user);
        db.setPassword(m_password);
        db.setDatabaseName(m_dbName);

        if (!db.open()) {
            qWarning() << "Pool connection" << connName << "open failed:" << db.lastError().text();
            continue;
        }

        ptp.available.insert(connName);
    }
}

DbGuard ConnectionPool::acquire(int timeoutMs)
{
    Qt::HANDLE tid = QThread::currentThreadId();

    QMutexLocker locker(&m_mutex);

    auto it = m_threadPools.find(tid);
    if (it == m_threadPools.end()) {
        // First time this thread requests a connection — create its pool
        PerThreadPool ptp;
        locker.unlock();
        createConnectionsForThread(ptp);
        locker.relock();
        m_threadPools[tid] = ptp;
        it = m_threadPools.find(tid);
    }

    while (it->available.isEmpty()) {
        if (!m_cond.wait(&m_mutex, timeoutMs)) {
            qWarning() << "ConnectionPool::acquire timeout on thread" << reinterpret_cast<quintptr>(tid);
            return DbGuard();
        }
    }

    QString name = *it->available.begin();
    it->available.remove(name);
    it->busy.insert(name);

    return DbGuard(name, this);
}

void ConnectionPool::release(const QString& name)
{
    Qt::HANDLE tid = QThread::currentThreadId();

    QMutexLocker locker(&m_mutex);
    auto it = m_threadPools.find(tid);
    if (it != m_threadPools.end()) {
        it->busy.remove(name);
        it->available.insert(name);
        m_cond.wakeOne();
    }
}
