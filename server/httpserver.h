#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <memory>
#include <QtConcurrent>

class ServerDb;

class HttpServer : public QObject {
    Q_OBJECT
public:
    explicit HttpServer(ServerDb* db, quint16 port, QObject* parent = nullptr);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    struct Request {
        QString method;
        QString path;
        QMap<QString, QString> headers;
        QMap<QString, QString> query;
        QByteArray body;
    };

    QTcpServer* m_server;
    ServerDb* m_db;
    QMap<QTcpSocket*, QByteArray> m_buffers;

    // Parsing
    Request parseRequest(const QByteArray& raw, bool& complete);
    QMap<QString, QString> parseQueryString(const QString& query) const;
    static bool parseMultipart(const QByteArray& body, const QString& boundary,
                               QMap<QString, QByteArray>& fields,
                               QMap<QString, QByteArray>& files);

    // Dispatch
    void dispatch(QTcpSocket* socket, const Request& req);

    // Response helpers
    void sendJson(QTcpSocket* socket, int statusCode, const QJsonObject& obj);
    void sendBinary(QTcpSocket* socket, const QByteArray& data, const QString& contentType);
    void sendError(QTcpSocket* socket, int statusCode, const QString& msg);
    void discardSocket(QTcpSocket* socket);

    // DB runner — same pattern as ChatServer
    template<typename DbFunc, typename Callback>
    void runDbAsync(DbFunc dbWork, Callback callback)
    {
        QtConcurrent::run([this, dbWork, callback]() {
            dbWork();
            QMetaObject::invokeMethod(this, callback, Qt::QueuedConnection);
        });
    }

    // Route handlers
    void handleLogin(QTcpSocket* socket, const Request& req);
    void handleRegister(QTcpSocket* socket, const Request& req);
    void handleGetFriends(QTcpSocket* socket, const Request& req);
    void handleAddFriend(QTcpSocket* socket, const Request& req);
    void handleAcceptFriend(QTcpSocket* socket, const Request& req);
    void handleRejectFriend(QTcpSocket* socket, const Request& req);
    void handleChatHistory(QTcpSocket* socket, const Request& req);
    void handleGetProfile(QTcpSocket* socket, const Request& req);
    void handleSaveProfile(QTcpSocket* socket, const Request& req);
    void handleUploadAvatar(QTcpSocket* socket, const Request& req);
    void handleGetAvatar(QTcpSocket* socket, const Request& req);
    void handleGetMovieReviews(QTcpSocket* socket, const Request& req);
    void handleGetUserReviews(QTcpSocket* socket, const Request& req);
    void handleSaveReview(QTcpSocket* socket, const Request& req);
    void handleDeleteReview(QTcpSocket* socket, const Request& req);
    void handleGetWishList(QTcpSocket* socket, const Request& req);
    void handleGetWatchedList(QTcpSocket* socket, const Request& req);
};
