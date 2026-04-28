#pragma once
#include <QString>
#include <QStringList>
#include <QList>

struct PersonInfo {
    QString name;
    QString nameCn;
    QString nameEn;
};

struct MovieLangData {
    QString name;
    QString poster;
    QString genre;
    QString description;
    QString language;
    QString country;
    QString lang;
};

struct Movie {
    QString originalName;
    QString alias;
    QString year;
    QString type;
    int duration = 0;
    int episodes = 0;
    int totalSeasons = 0;
    QString dateReleased;
    QString doubanId;
    QString imdbId;

    double doubanRating = 0.0;
    int doubanVotes = 0;
    double imdbRating = 0.0;
    int imdbVotes = 0;
    double rottenRating = 0.0;
    int rottenVotes = 0;

    QList<MovieLangData> langData;
    QList<PersonInfo> directors;
    QList<PersonInfo> writers;
    QList<PersonInfo> actors;

    // 本地用户数据
    double userRating = 0.0;
    QString userReview;
    bool isWished = false;
    bool isWatched = false;

    QString getName() const {
        for (const auto& d : langData) {
            if (d.lang == "Cn") return d.name;
        }
        return originalName;
    }

    QString getPoster() const {
        for (const auto& d : langData) {
            if (d.lang == "Cn" && !d.poster.isEmpty()) return d.poster;
        }
        for (const auto& d : langData) {
            if (!d.poster.isEmpty()) return d.poster;
        }
        return QString();
    }

    QString getGenre() const {
        for (const auto& d : langData) {
            if (d.lang == "Cn") return d.genre;
        }
        return QString();
    }

    QString getDescription() const {
        for (const auto& d : langData) {
            if (d.lang == "Cn") return d.description;
        }
        return QString();
    }

    QString getCountry() const {
        for (const auto& d : langData) {
            if (d.lang == "Cn") return d.country;
        }
        return QString();
    }

    int getDurationMinutes() const {
        return duration / 60;
    }
};

struct UserReview {
    int id = 0;
    QString doubanId;
    QString movieName;
    QString username;
    double rating = 0.0;
    QString content;
    QString createTime;
    bool isWished = false;
    bool isWatched = false;
    QString posterUrl;
};

struct SearchResult {
    int total = 0;
    int page = 1;
    int limit = 10;
    int skip = 0;
    int count = 0;
    int totalPages = 0;
    bool hasMore = false;
    QList<Movie> movies;
};
