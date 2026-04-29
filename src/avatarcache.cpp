#include "avatarcache.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>

static QString baseDir()
{
    QString dir = QCoreApplication::applicationDirPath() + "/cache/avatars";
    QDir().mkpath(dir);
    return dir;
}

QString AvatarCache::filePath(int userId)
{
    return baseDir() + QString("/%1.png").arg(userId);
}

bool AvatarCache::exists(int userId)
{
    return QFile::exists(filePath(userId));
}

QPixmap AvatarCache::get(int userId)
{
    if (userId == 0) { qDebug() << "[AvatarCache] get: userId=0, skip"; return QPixmap(); }
    QString path = filePath(userId);
    bool ok = QFile::exists(path);
    qDebug() << "[AvatarCache] get userId=" << userId << "path=" << path << "exists=" << ok;
    if (ok) {
        QPixmap pm;
        if (pm.load(path)) {
            qDebug() << "[AvatarCache] get: loaded" << pm.size();
            return pm;
        }
    }
    return QPixmap();
}

void AvatarCache::put(int userId, const QByteArray& data)
{
    qDebug() << "[AvatarCache] put userId=" << userId << "size=" << data.size();
    if (userId == 0 || data.isEmpty()) { qDebug() << "[AvatarCache] put: skip (uid=0 or empty)"; return; }
    QString path = filePath(userId);
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(data);
        f.close();
        qDebug() << "[AvatarCache] put: written to" << path;
    } else {
        qDebug() << "[AvatarCache] put: FAILED to write" << path;
    }
}
