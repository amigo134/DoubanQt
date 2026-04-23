#include "apimanager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QUrl>
#include <QSslSocket>
#include <QSslError>
#include <QDebug>

const QString ApiManager::BASE_URL = "https://api.wmdb.tv";
const QString ApiManager::SEARCH_URL = "https://api.wmdb.tv/api/v1/movie/search";
const QString ApiManager::DETAIL_URL = "https://api.wmdb.tv/movie/api";
const QString ApiManager::TOP250_URL = "https://api.wmdb.tv/api/v1/top";

ApiManager::ApiManager(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    qDebug() << "SSL supported:" << QSslSocket::supportsSsl()
             << "  SSL version:" << QSslSocket::sslLibraryVersionString();
}

void ApiManager::searchMovies(const QString& query, const QString& actor,
                               int year, int limit, int skip, const QString& lang)
{
    QUrl url(SEARCH_URL);
    QUrlQuery urlQuery;

    if (!query.isEmpty())
        urlQuery.addQueryItem("q", query);
    if (!actor.isEmpty())
        urlQuery.addQueryItem("actor", actor);
    if (year > 0)
        urlQuery.addQueryItem("year", QString::number(year));
    urlQuery.addQueryItem("limit", QString::number(limit));
    urlQuery.addQueryItem("skip", QString::number(skip));
    urlQuery.addQueryItem("lang", lang);

    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "Mozilla/5.0 DoubanQt/1.0");
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);

    emit networkBusy(true);
    QNetworkReply* reply = m_nam->get(request);
    connect(reply, &QNetworkReply::sslErrors, reply,
            [reply](const QList<QSslError>&){ reply->ignoreSslErrors(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSearchReply(reply);
    });
}

void ApiManager::getMovieById(const QString& doubanId)
{
    QUrl url(DETAIL_URL);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("id", doubanId);
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "Mozilla/5.0 DoubanQt/1.0");
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);

    emit networkBusy(true);
    QNetworkReply* reply = m_nam->get(request);
    connect(reply, &QNetworkReply::sslErrors, reply,
            [reply](const QList<QSslError>&){ reply->ignoreSslErrors(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onDetailReply(reply);
    });
}

void ApiManager::getTop250(const QString& type, int limit, int skip, const QString& lang)
{
    QUrl url(TOP250_URL);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("type", type);
    urlQuery.addQueryItem("limit", QString::number(limit));
    urlQuery.addQueryItem("skip", QString::number(skip));
    urlQuery.addQueryItem("lang", lang);
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "Mozilla/5.0 DoubanQt/1.0");
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);

    emit networkBusy(true);
    QNetworkReply* reply = m_nam->get(request);
    connect(reply, &QNetworkReply::sslErrors, reply,
            [reply](const QList<QSslError>&){ reply->ignoreSslErrors(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onTop250Reply(reply);
    });
}

void ApiManager::onSearchReply(QNetworkReply* reply)
{
    emit networkBusy(false);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit errorOccurred("无效的响应格式");
        return;
    }

    QJsonObject root = doc.object();
    SearchResult result;
    result.total = root["total"].toInt();
    result.page = root["page"].toInt(1);
    result.limit = root["limit"].toInt(10);
    result.skip = root["skip"].toInt(0);
    result.count = root["count"].toInt();
    result.totalPages = root["totalPages"].toInt();
    result.hasMore = root["hasMore"].toBool();

    QJsonArray dataArr = root["data"].toArray();
    for (const QJsonValue& val : dataArr) {
        if (val.isObject()) {
            result.movies.append(parseMovie(val.toObject()));
        }
    }

    emit searchResultReady(result);
}

void ApiManager::onDetailReply(QNetworkReply* reply)
{
    emit networkBusy(false);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    Movie movie;
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        if (!arr.isEmpty() && arr[0].isObject()) {
            movie = parseMovie(arr[0].toObject());
        }
    } else if (doc.isObject()) {
        movie = parseMovie(doc.object());
    } else {
        emit errorOccurred("无效的响应格式");
        return;
    }

    emit movieDetailReady(movie);
}

void ApiManager::onTop250Reply(QNetworkReply* reply)
{
    emit networkBusy(false);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    QList<Movie> movies;
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            if (val.isObject()) {
                movies.append(parseMovie(val.toObject()));
            }
        }
    } else if (doc.isObject()) {
        QJsonObject root = doc.object();
        QJsonArray dataArr = root["data"].toArray();
        for (const QJsonValue& val : dataArr) {
            if (val.isObject()) {
                movies.append(parseMovie(val.toObject()));
            }
        }
    }

    emit top250Ready(movies);
}

Movie ApiManager::parseMovie(const QJsonObject& obj)
{
    Movie movie;
    movie.originalName = obj["originalName"].toString();
    movie.alias = obj["alias"].toString();
    movie.year = obj["year"].toString();
    movie.type = obj["type"].toString();
    movie.duration = obj["duration"].toInt();
    movie.episodes = obj["episodes"].toInt();
    movie.totalSeasons = obj["totalSeasons"].toInt();
    movie.dateReleased = obj["dateReleased"].toString();
    movie.doubanId = obj["doubanId"].toString();
    movie.imdbId = obj["imdbId"].toString();

    movie.doubanRating = obj["doubanRating"].toString().toDouble();
    movie.doubanVotes = obj["doubanVotes"].toInt();
    movie.imdbRating = obj["imdbRating"].toString().toDouble();
    movie.imdbVotes = obj["imdbVotes"].toInt();
    movie.rottenRating = obj["rottenRating"].toString().toDouble();
    movie.rottenVotes = obj["rottenVotes"].toInt();

    QJsonArray langArr = obj["data"].toArray();
    for (const QJsonValue& lv : langArr) {
        if (!lv.isObject()) continue;
        QJsonObject lo = lv.toObject();
        MovieLangData ld;
        ld.name = lo["name"].toString();
        ld.poster = lo["poster"].toString();
        ld.genre = lo["genre"].toString();
        ld.description = lo["description"].toString();
        ld.language = lo["language"].toString();
        ld.country = lo["country"].toString();
        ld.lang = lo["lang"].toString();
        movie.langData.append(ld);
    }

    auto parsePerson = [](const QJsonArray& arr) -> QList<PersonInfo> {
        QList<PersonInfo> persons;
        for (const QJsonValue& pv : arr) {
            if (!pv.isObject()) continue;
            QJsonArray pdata = pv.toObject()["data"].toArray();
            PersonInfo pi;
            for (const QJsonValue& pdv : pdata) {
                if (!pdv.isObject()) continue;
                QJsonObject pdo = pdv.toObject();
                QString lang = pdo["lang"].toString();
                QString name = pdo["name"].toString();
                if (lang == "Cn") pi.nameCn = name;
                else if (lang == "En") pi.nameEn = name;
                if (pi.name.isEmpty()) pi.name = name;
            }
            if (!pi.nameCn.isEmpty()) pi.name = pi.nameCn;
            else if (!pi.nameEn.isEmpty()) pi.name = pi.nameEn;
            persons.append(pi);
        }
        return persons;
    };

    movie.directors = parsePerson(obj["director"].toArray());
    movie.writers = parsePerson(obj["writer"].toArray());
    movie.actors = parsePerson(obj["actor"].toArray());

    return movie;
}
