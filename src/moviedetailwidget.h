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

class MovieDetailWidget : public QWidget {
    Q_OBJECT
public:
    explicit MovieDetailWidget(DatabaseManager* db, QWidget* parent = nullptr);

    void setMovie(const Movie& movie);

signals:
    void backRequested();
    void reviewUpdated();

private slots:
    void onWriteReview();
    void onToggleWish();
    void onToggleWatched();

private:
    void loadPoster(const QString& url);
    void updateUserSection();
    void buildUI();
    void updateTopLayout();

    DatabaseManager* m_db;
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
    QLabel* m_actorLabel;
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
    bool m_isVerticalLayout = false;
    static constexpr int NARROW_THRESHOLD = 560;
};
