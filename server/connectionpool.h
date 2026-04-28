#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QMutex>
#include <QWaitCondition>
#include <QSet>
#include <QMap>
#include <QThread>

class ConnectionPool;

class DbGuard {
public:
    DbGuard();
    DbGuard(const QString& name, ConnectionPool* pool);
    ~DbGuard();
    DbGuard(DbGuard&& other);
    DbGuard& operator=(DbGuard&& other);
    DbGuard(const DbGuard&) = delete;
    DbGuard& operator=(const DbGuard&) = delete;

    QString name() const { return m_name; }
    bool valid() const { return !m_name.isEmpty(); }

private:
    QString m_name;
    ConnectionPool* m_pool = nullptr;
};

class ConnectionPool : public QObject {
    Q_OBJECT
public:
    explicit ConnectionPool(int poolSize = 5, QObject* parent = nullptr);
    ~ConnectionPool() override;

    bool initialize(const QString& host, int port,
                    const QString& user, const QString& password,
                    const QString& dbName);
    DbGuard acquire(int timeoutMs = 30000);

private:
    struct PerThreadPool {
        QSet<QString> available;
        QSet<QString> busy;
    };

    void createConnectionsForThread(PerThreadPool& ptp);
    void release(const QString& name);
    friend class DbGuard;

    QMap<Qt::HANDLE, PerThreadPool> m_threadPools;
    QMutex m_mutex;
    QWaitCondition m_cond;
    int m_poolSize;

    QString m_host;
    int m_port = 3306;
    QString m_user;
    QString m_password;
    QString m_dbName;
};
