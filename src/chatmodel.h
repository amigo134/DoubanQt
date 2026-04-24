#pragma once
#include <QString>
#include <QList>

struct FriendInfo {
    QString username;
    bool online = false;
};

struct ChatMsg {
    QString from;
    QString to;
    QString content;
    QString time;
    bool isOwn = false;
};
