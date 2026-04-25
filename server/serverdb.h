#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QList>

struct ServerMsg {
    int id;
    int fromId;
    int toId;
    QString content;
    QString time;
    bool delivered;
};

class ServerDb : public QObject {
    Q_OBJECT
public:
    explicit ServerDb(QObject* parent = nullptr);
    bool initialize();

    int registerUser(const QString& username, const QString& passwordHash);
    int loginUser(const QString& username, const QString& passwordHash);
    int getUserId(const QString& username);
    QString getUsername(int userId);

    bool addFriendRequest(int fromId, int toId);
    bool acceptFriend(int fromId, int toId);
    bool rejectFriend(int fromId, int toId);
    QList<int> getFriendIds(int userId);
    QList<int> getPendingFromIds(int userId);
    int getFriendshipStatus(int userId, int friendId);

    bool saveMessage(int fromId, int toId, const QString& content, const QString& time, int* outMsgId = nullptr);
    QList<ServerMsg> getOfflineMessages(int userId);
    QList<ServerMsg> getChatHistory(int userId, int friendId, int limit, int beforeMsgId);
    void markDelivered(int userId);

private:
    QSqlDatabase m_db;
    bool createTables();
};
