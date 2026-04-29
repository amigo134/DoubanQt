#pragma once
#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QResizeEvent>
#include <QGridLayout>
#include "moviemodel.h"
#include "ratingwidget.h"
#include "databasemanager.h"

class ChatManager;
class ServerApiClient;

class MovieDetailWidget : public QWidget {
    Q_OBJECT
public:
    explicit MovieDetailWidget(DatabaseManager* db, ChatManager* chatMgr, ServerApiClient* serverApi, QWidget* parent = nullptr);

    void setMovie(const Movie& movie);

signals:
    void backRequested();
    void reviewUpdated();

private slots:
    void onWriteReview();
    void onToggleWish();
    void onToggleWatched();
    void onReviewReceived(const UserReview& review);
    void onMovieReviewsReceived(const QString& doubanId, const QList<UserReview>& reviews);

private:
    void loadPoster(const QString& url);
    void updateUserSection();
    void refreshPublicReviews(const QList<UserReview>& reviews);
    void buildUI();
    void updateTopLayout();

    DatabaseManager* m_db;
    ChatManager* m_chatMgr;
    ServerApiClient* m_serverApi;
    Movie m_movie;
    UserReview m_userReview;

    QLabel* m_posterLabel;
    QLabel* m_titleLabel;
    QLabel* m_originalTitleLabel;
    QLabel* m_yearLabel;
    QLabel* m_durationLabel;
    QLabel* m_genreLabel;
    QLabel* m_countryLabel;
    QLabel* m_directorLabel;
    QLabel* m_writerLabel;
    QLabel* m_actorLabel;
    QLabel* m_languageLabel;
    QLabel* m_dateReleasedLabel;
    QLabel* m_aliasLabel;
    QLabel* m_descLabel;

    QLabel* m_doubanRatingLabel;
    QLabel* m_imdbRatingLabel;
    QLabel* m_rottenRatingLabel;

    RatingWidget* m_userRatingWidget;
    QLabel* m_userRatingTextLabel;
    QLabel* m_userReviewLabel;

    QPushButton* m_wishBtn;
    QPushButton* m_watchedBtn;
    QPushButton* m_reviewBtn;

    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;

    QWidget* m_topWidget;
    QGridLayout* m_topGrid;
    QWidget* m_infoWidget;
    QVBoxLayout* m_infoLayout;

    // Public reviews section
    QWidget* m_publicReviewsWidget;
    QVBoxLayout* m_publicReviewsLayout;

    bool m_isVerticalLayout = false;
    static constexpr int NARROW_THRESHOLD = 560;
};
