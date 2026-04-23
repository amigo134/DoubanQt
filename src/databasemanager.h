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

    bool setWished(const QString& doubanId, const QString& movieName, bool wished);
    bool setWatched(const QString& doubanId, const QString& movieName, bool watched);
    QList<UserReview> getWishList();
    QList<UserReview> getWatchedList();

private:
    QSqlDatabase m_db;
    bool createTables();
};
