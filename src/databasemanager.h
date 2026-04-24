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

private:
    QSqlDatabase m_db;
    bool createTables();
};
