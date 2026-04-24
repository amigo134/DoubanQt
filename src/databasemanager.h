#pragma once
#include <QObject>
#include <QSqlDatabase>
#include "moviemodel.h"

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    bool initialize();
    bool isReady() const;
    bool hasUsers();
    int registerUser(const QString& username, const QString& password);
    int loginUser(const QString& username, const QString& password);
    void setCurrentUser(int userId);
    int currentUserId() const;
    QString currentUsername();

    bool saveReview(const UserReview& review);
    UserReview getReview(const QString& doubanId);
    QList<UserReview> getAllReviews();
    bool deleteReview(const QString& doubanId);
    void updatePosterUrl(const QString& doubanId, const QString& posterUrl);

    bool setWished(const QString& doubanId, const QString& movieName, bool wished, const QString& posterUrl = QString());
    bool setWatched(const QString& doubanId, const QString& movieName, bool watched, const QString& posterUrl = QString());
    QList<UserReview> getWishList();
    QList<UserReview> getWatchedList();

    QString getProfileName();
    QString getProfileBio();
    QString getAvatarPath();
    void saveProfile(const QString& name, const QString& bio);
    void saveAvatarPath(const QString& path);
    void ensureProfile();

private:
    QSqlDatabase m_db;
    int m_currentUserId = 0;
    bool m_ready = false;
    bool createTables();
};
