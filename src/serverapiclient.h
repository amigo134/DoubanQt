#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include "moviemodel.h"
#include "chatmodel.h"

class ServerApiClient : public QObject {
    Q_OBJECT
public:
    explicit ServerApiClient(QObject* parent = nullptr);
    void setBaseUrl(const QString& url);

    void registerUser(const QString& username, const QString& password);

    // Friends
    void getFriendList(int userId);
    void addFriend(int userId, const QString& targetUsername);
    void acceptFriend(int userId, const QString& fromUsername);
    void rejectFriend(int userId, const QString& fromUsername);

    // Chat history
    void getChatHistory(int userId, const QString& withUser, int limit, int beforeMsgId);

    // Profile
    void getProfile(int userId);
    void saveProfile(int userId, const QString& name, const QString& bio);
    void uploadAvatar(int userId, const QString& filePath);

    // Reviews
    void saveReview(int userId, const QString& doubanId, const QString& movieName,
                    double rating, const QString& content, bool isWished, bool isWatched,
                    const QString& posterUrl);
    void getReview(int userId, const QString& doubanId);
    void getAllReviews(int userId);
    void deleteReview(int userId, const QString& doubanId);
    void getWishList(int userId);
    void getWatchedList(int userId);
    void getMovieReviews(const QString& doubanId);
    void getUserReviews(int userId);

signals:
    void registerResult(bool success, int userId, const QString& errorMsg);

    // Friends — match ChatManager signal names
    void friendListReceived(const QList<FriendInfo>& friends);
    void addFriendResult(bool success, const QString& message);
    void acceptFriendResult(bool success);
    void rejectFriendResult(bool success);

    // Chat history
    void chatHistoryReceived(const QString& with, const QList<ChatMsg>& messages, bool hasMore);

    // Profile — match ChatManager signal names
    void profileReceived(const QString& name, const QString& bio, const QString& avatarPath);
    void profileSaved(bool success);
    void avatarSaved(bool success);

    // Reviews — match ChatManager signal names
    void reviewSaved(bool success, const QString& doubanId);
    void reviewReceived(const UserReview& review);
    void reviewsListReceived(const QList<UserReview>& reviews);
    void reviewDeleted(bool success, const QString& doubanId);
    void wishListReceived(const QList<UserReview>& list);
    void watchedListReceived(const QList<UserReview>& list);
    void movieReviewsReceived(const QString& doubanId, const QList<UserReview>& reviews);
    void userReviewsReceived(const QString& username, const QList<UserReview>& reviews);

private:
    QNetworkReply* post(const QString& path, const QJsonObject& body);
    QNetworkReply* put(const QString& path, const QJsonObject& body);
    QNetworkReply* del(const QString& path, const QJsonObject& body);
    QNetworkReply* get(const QString& path);

    QNetworkAccessManager* m_nam;
    QString m_baseUrl = "http://localhost:8766";
};
