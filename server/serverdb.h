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

struct ReviewData {
    int id = 0;
    int userId = 0;
    QString doubanId;
    QString movieName;
    double rating = 0;
    QString content;
    bool isWished = false;
    bool isWatched = false;
    QString posterUrl;
    QString createTime;
    QString updateTime;
};

struct ProfileData {
    QString name;
    QString bio;
    QString avatarPath;
};

class ConnectionPool;

class ServerDb : public QObject {
    Q_OBJECT
public:
    explicit ServerDb(int poolSize = 5, QObject* parent = nullptr);
    ~ServerDb() override;

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

    // Reviews
    bool saveReview(int userId, const QString& doubanId, const QString& movieName,
                    double rating, const QString& content, bool isWished, bool isWatched,
                    const QString& posterUrl);
    ReviewData getReview(int userId, const QString& doubanId);
    QList<ReviewData> getAllReviews(int userId);
    bool deleteReview(int userId, const QString& doubanId);
    QList<ReviewData> getWishList(int userId);
    QList<ReviewData> getWatchedList(int userId);

    // Profile
    ProfileData getProfile(int userId);
    bool saveProfile(int userId, const QString& name, const QString& bio);
    bool saveAvatarPath(int userId, const QString& avatarPath);

private:
    bool createTables(const QSqlDatabase& db);
    ConnectionPool* m_pool = nullptr;
};
