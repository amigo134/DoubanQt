#pragma once
#include <QPixmap>
#include <QString>

class AvatarCache {
public:
    static QPixmap get(int userId);
    static void put(int userId, const QByteArray& data);
    static bool exists(int userId);
    static QString filePath(int userId);
};
