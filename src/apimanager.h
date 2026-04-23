#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslConfiguration>
#include "moviemodel.h"

class ApiManager : public QObject {
    Q_OBJECT
public:
    explicit ApiManager(QObject* parent = nullptr);

    void searchMovies(const QString& query, const QString& actor = QString(),
                      int year = 0, int limit = 20, int skip = 0,
                      const QString& lang = "Cn");
    void getMovieById(const QString& doubanId);

signals:
    void searchResultReady(const SearchResult& result);
    void movieDetailReady(const Movie& movie);
    void errorOccurred(const QString& error);
    void networkBusy(bool busy);

private slots:
    void onSearchReply(QNetworkReply* reply);
    void onDetailReply(QNetworkReply* reply);

private:
    Movie parseMovie(const QJsonObject& obj);
    QNetworkAccessManager* m_nam;

    static const QString BASE_URL;
    static const QString SEARCH_URL;
    static const QString DETAIL_URL;
};
