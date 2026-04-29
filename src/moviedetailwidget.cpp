#include "moviedetailwidget.h"
#include "chatmanager.h"
#include "serverapiclient.h"
#include "reviewdialog.h"
#include "imagecache.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>

MovieDetailWidget::MovieDetailWidget(DatabaseManager* db, ChatManager* chatMgr, ServerApiClient* serverApi, QWidget* parent)
    : QWidget(parent)
    , m_db(db)
    , m_chatMgr(chatMgr)
    , m_serverApi(serverApi)
{
    buildUI();

    connect(m_serverApi, &ServerApiClient::reviewReceived,
            this, &MovieDetailWidget::onReviewReceived);
    connect(m_serverApi, &ServerApiClient::movieReviewsReceived,
            this, &MovieDetailWidget::onMovieReviewsReceived);
}

void MovieDetailWidget::buildUI()
{
    setStyleSheet(R"(
        MovieDetailWidget {
            background: #F7F7F9;
        }
        QLabel#sectionTitle {
            font-size: 16px;
            font-weight: bold;
            color: #333;
            border-left: 3px solid #00B51D;
            padding-left: 8px;
            margin-top: 8px;
        }
        QLabel#metaLabel {
            font-size: 13px;
            color: #555;
            line-height: 1.6;
        }
        QPushButton#backBtn {
            background: transparent;
            color: #00B51D;
            border: none;
            font-size: 14px;
            font-weight: bold;
            text-align: left;
            padding: 6px 0;
        }
        QPushButton#backBtn:hover {
            color: #009A18;
        }
        QPushButton#wishBtn {
            background: #FFF5F5;
            color: #E74C3C;
            border: 1px solid #FFD4D4;
            border-radius: 6px;
            padding: 8px 20px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton#wishBtn:checked, QPushButton#wishBtn:hover {
            background: #E74C3C;
            color: white;
            border-color: #E74C3C;
        }
        QPushButton#watchedBtn {
            background: #F0FFF0;
            color: #00B51D;
            border: 1px solid #B8E6B8;
            border-radius: 6px;
            padding: 8px 20px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton#watchedBtn:checked, QPushButton#watchedBtn:hover {
            background: #00B51D;
            color: white;
            border-color: #00B51D;
        }
        QPushButton#reviewBtn {
            background: #00B51D;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 24px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton#reviewBtn:hover {
            background: #009A18;
        }
        QPushButton#reviewBtn:pressed {
            background: #008012;
        }
        QFrame#ratingCard {
            background: white;
            border-radius: 10px;
            border: 1px solid #EEE;
        }
    )");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* navBar = new QWidget();
    navBar->setStyleSheet("background: white; border-bottom: 1px solid #E8E8EC;");
    navBar->setFixedHeight(50);
    auto* navLayout = new QHBoxLayout(navBar);
    navLayout->setContentsMargins(24, 0, 24, 0);

    auto* backBtn = new QPushButton("← 返回");
    backBtn->setObjectName("backBtn");
    connect(backBtn, &QPushButton::clicked, this, &MovieDetailWidget::backRequested);
    navLayout->addWidget(backBtn);
    navLayout->addStretch();
    mainLayout->addWidget(navBar);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->verticalScrollBar()->setStyleSheet(R"(
        QScrollBar:vertical { width: 6px; background: transparent; }
        QScrollBar::handle:vertical { background: rgba(0,0,0,0.10); border-radius: 3px; }
        QScrollBar::handle:vertical:hover { background: rgba(0,0,0,0.20); }
    )");

    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background: #F7F7F9;");
    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);

    auto* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(32, 24, 32, 28);
    contentLayout->setSpacing(18);

    m_topWidget = new QWidget();
    m_topWidget->setStyleSheet("background: white; border-radius: 10px; border: 1px solid #EEE;");
    m_topGrid = new QGridLayout(m_topWidget);
    m_topGrid->setContentsMargins(28, 28, 28, 28);
    m_topGrid->setSpacing(28);

    m_posterLabel = new QLabel();
    m_posterLabel->setFixedSize(170, 235);
    m_posterLabel->setAlignment(Qt::AlignCenter);
    m_posterLabel->setStyleSheet("background: #E8E8EC; border-radius: 10px;");

    m_infoWidget = new QWidget();
    m_infoLayout = new QVBoxLayout(m_infoWidget);
    m_infoLayout->setContentsMargins(0, 0, 0, 0);
    m_infoLayout->setSpacing(8);

    m_titleLabel = new QLabel();
    m_titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #222;");
    m_titleLabel->setWordWrap(true);
    m_infoLayout->addWidget(m_titleLabel);

    m_originalTitleLabel = new QLabel();
    m_originalTitleLabel->setStyleSheet("font-size: 13px; color: #999;");
    m_infoLayout->addWidget(m_originalTitleLabel);

    auto* metaFrame = new QFrame();
    metaFrame->setStyleSheet("background: #F9F9FB; border-radius: 8px;");
    auto* metaLayout = new QGridLayout(metaFrame);
    metaLayout->setContentsMargins(16, 14, 16, 14);
    metaLayout->setVerticalSpacing(6);
    metaLayout->setHorizontalSpacing(12);

    auto makeMetaLabel = [](const QString& text) {
        auto* l = new QLabel(text);
        l->setObjectName("metaLabel");
        return l;
    };

    m_yearLabel = makeMetaLabel("");
    m_durationLabel = makeMetaLabel("");
    m_genreLabel = makeMetaLabel("");
    m_countryLabel = makeMetaLabel("");
    m_languageLabel = makeMetaLabel("");
    m_dateReleasedLabel = makeMetaLabel("");
    m_directorLabel = makeMetaLabel("");
    m_directorLabel->setWordWrap(true);
    m_writerLabel = makeMetaLabel("");
    m_writerLabel->setWordWrap(true);
    m_actorLabel = makeMetaLabel("");
    m_actorLabel->setWordWrap(true);
    m_aliasLabel = makeMetaLabel("");
    m_aliasLabel->setWordWrap(true);

    auto makeMetaKey = [](const QString& text) {
        auto* l = new QLabel(text);
        l->setStyleSheet("font-size: 12px; color: #AAA; font-weight: bold;");
        return l;
    };

    int row = 0;
    metaLayout->addWidget(makeMetaKey("年份"), row, 0);
    metaLayout->addWidget(m_yearLabel, row, 1);
    row++;
    metaLayout->addWidget(makeMetaKey("时长"), row, 0);
    metaLayout->addWidget(m_durationLabel, row, 1);
    row++;
    metaLayout->addWidget(makeMetaKey("类型"), row, 0);
    metaLayout->addWidget(m_genreLabel, row, 1);
    row++;
    metaLayout->addWidget(makeMetaKey("地区"), row, 0);
    metaLayout->addWidget(m_countryLabel, row, 1);
    row++;
    metaLayout->addWidget(makeMetaKey("语言"), row, 0);
    metaLayout->addWidget(m_languageLabel, row, 1);
    row++;
    metaLayout->addWidget(makeMetaKey("上映"), row, 0);
    metaLayout->addWidget(m_dateReleasedLabel, row, 1);
    row++;
    metaLayout->addWidget(makeMetaKey("导演"), row, 0);
    metaLayout->addWidget(m_directorLabel, row, 1);
    row++;
    metaLayout->addWidget(makeMetaKey("编剧"), row, 0, 1, 1, Qt::AlignTop);
    metaLayout->addWidget(m_writerLabel, row, 1);
    row++;
    metaLayout->addWidget(makeMetaKey("主演"), row, 0, 1, 1, Qt::AlignTop);
    metaLayout->addWidget(m_actorLabel, row, 1);
    row++;
    metaLayout->addWidget(makeMetaKey("又名"), row, 0, 1, 1, Qt::AlignTop);
    metaLayout->addWidget(m_aliasLabel, row, 1);
    row++;
    metaLayout->setColumnStretch(1, 1);
    m_infoLayout->addWidget(metaFrame);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    m_wishBtn = new QPushButton("♡ 想看");
    m_wishBtn->setObjectName("wishBtn");
    m_wishBtn->setCheckable(true);
    m_wishBtn->setStyleSheet(R"(
        QPushButton { background: #FFF5F5; color: #E74C3C; border: 1px solid #FFD4D4; border-radius: 6px; padding: 8px 20px; font-size: 13px; font-weight: bold; }
        QPushButton:checked, QPushButton:hover { background: #E74C3C; color: white; border-color: #E74C3C; }
    )");
    m_watchedBtn = new QPushButton("✓ 看过");
    m_watchedBtn->setObjectName("watchedBtn");
    m_watchedBtn->setCheckable(true);
    m_watchedBtn->setStyleSheet(R"(
        QPushButton { background: #F0FFF0; color: #00B51D; border: 1px solid #B8E6B8; border-radius: 6px; padding: 8px 20px; font-size: 13px; font-weight: bold; }
        QPushButton:checked, QPushButton:hover { background: #00B51D; color: white; border-color: #00B51D; }
    )");
    m_reviewBtn = new QPushButton("✏ 写短评");
    m_reviewBtn->setObjectName("reviewBtn");
    m_reviewBtn->setStyleSheet(R"(
        QPushButton { background: #00B51D; color: white; border: none; border-radius: 6px; padding: 8px 24px; font-size: 13px; font-weight: bold; }
        QPushButton:hover { background: #009A18; }
        QPushButton:pressed { background: #008012; }
    )");

    connect(m_wishBtn, &QPushButton::clicked, this, &MovieDetailWidget::onToggleWish);
    connect(m_watchedBtn, &QPushButton::clicked, this, &MovieDetailWidget::onToggleWatched);
    connect(m_reviewBtn, &QPushButton::clicked, this, &MovieDetailWidget::onWriteReview);

    btnLayout->addWidget(m_wishBtn);
    btnLayout->addWidget(m_watchedBtn);
    btnLayout->addWidget(m_reviewBtn);
    btnLayout->addStretch();
    m_infoLayout->addLayout(btnLayout);
    m_infoLayout->addStretch();

    m_topGrid->addWidget(m_posterLabel, 0, 0, Qt::AlignTop);
    m_topGrid->addWidget(m_infoWidget, 0, 1);
    m_topGrid->setColumnStretch(1, 1);

    contentLayout->addWidget(m_topWidget);

    auto* ratingCard = new QFrame();
    ratingCard->setObjectName("ratingCard");
    auto* ratingLayout = new QHBoxLayout(ratingCard);
    ratingLayout->setContentsMargins(24, 20, 24, 20);
    ratingLayout->setSpacing(0);

    auto makePlatformRating = [&](const QString& platform, QLabel*& ratingLabel, const QString& color, const QString& icon) {
        auto* col = new QVBoxLayout();
        col->setSpacing(2);
        auto* platformLabel = new QLabel(platform);
        platformLabel->setAlignment(Qt::AlignCenter);
        platformLabel->setStyleSheet("font-size: 11px; color: #999;");
        auto* iconLabel = new QLabel(icon);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("font-size: 18px;");
        ratingLabel = new QLabel("--");
        ratingLabel->setAlignment(Qt::AlignCenter);
        ratingLabel->setStyleSheet(QString("font-size: 26px; font-weight: bold; color: %1;").arg(color));
        col->addWidget(iconLabel);
        col->addWidget(ratingLabel);
        col->addWidget(platformLabel);
        return col;
    };

    ratingLayout->addLayout(makePlatformRating("豆瓣", m_doubanRatingLabel, "#00B51D", "🎬"));
    auto* divider1 = new QFrame();
    divider1->setFrameShape(QFrame::VLine);
    divider1->setStyleSheet("color: #EEE;");
    ratingLayout->addWidget(divider1);
    ratingLayout->addLayout(makePlatformRating("IMDb", m_imdbRatingLabel, "#F5A623", "🌟"));
    auto* divider2 = new QFrame();
    divider2->setFrameShape(QFrame::VLine);
    divider2->setStyleSheet("color: #EEE;");
    ratingLayout->addWidget(divider2);
    ratingLayout->addLayout(makePlatformRating("烂番茄", m_rottenRatingLabel, "#E74C3C", "🍅"));

    contentLayout->addWidget(ratingCard);

    auto* descTitle = new QLabel("剧情简介");
    descTitle->setObjectName("sectionTitle");
    contentLayout->addWidget(descTitle);

    m_descLabel = new QLabel();
    m_descLabel->setWordWrap(true);
    m_descLabel->setStyleSheet("font-size: 14px; color: #555; line-height: 1.8; background: white; border-radius: 10px; padding: 20px; border: 1px solid #EEE;");
    m_descLabel->setTextFormat(Qt::PlainText);
    contentLayout->addWidget(m_descLabel);

    auto* mySection = new QLabel("我的评价");
    mySection->setObjectName("sectionTitle");
    contentLayout->addWidget(mySection);

    auto* userRatingCard = new QFrame();
    userRatingCard->setObjectName("ratingCard");
    auto* userRatingLayout = new QVBoxLayout(userRatingCard);
    userRatingLayout->setContentsMargins(24, 20, 24, 20);
    userRatingLayout->setSpacing(8);

    auto* ratingRow = new QHBoxLayout();
    m_userRatingWidget = new RatingWidget(this);
    m_userRatingWidget->setStarSize(28);
    m_userRatingWidget->setReadOnly(true);

    m_userRatingTextLabel = new QLabel("尚未评分");
    m_userRatingTextLabel->setStyleSheet("font-size: 14px; color: #00B51D; margin-left: 12px; font-weight: bold;");
    ratingRow->addWidget(m_userRatingWidget);
    ratingRow->addWidget(m_userRatingTextLabel);
    ratingRow->addStretch();
    userRatingLayout->addLayout(ratingRow);

    m_userReviewLabel = new QLabel();
    m_userReviewLabel->setWordWrap(true);
    m_userReviewLabel->setStyleSheet("font-size: 14px; color: #555; border-left: 3px solid #00B51D; padding-left: 10px; margin-top: 4px;");
    m_userReviewLabel->setVisible(false);
    userRatingLayout->addWidget(m_userReviewLabel);

    contentLayout->addWidget(userRatingCard);

    // --- Public reviews section ---
    auto* publicTitle = new QLabel("全部评价");
    publicTitle->setObjectName("sectionTitle");
    contentLayout->addWidget(publicTitle);

    m_publicReviewsWidget = new QWidget();
    m_publicReviewsWidget->setStyleSheet("background: white; border-radius: 10px; padding: 16px; border: 1px solid #EEE;");
    m_publicReviewsLayout = new QVBoxLayout(m_publicReviewsWidget);
    m_publicReviewsLayout->setContentsMargins(0, 0, 0, 0);
    m_publicReviewsLayout->setSpacing(12);
    contentLayout->addWidget(m_publicReviewsWidget);

    contentLayout->addStretch();
}

void MovieDetailWidget::updateTopLayout()
{
    bool shouldVertical = width() < NARROW_THRESHOLD;
    if (shouldVertical == m_isVerticalLayout) return;
    m_isVerticalLayout = shouldVertical;

    m_topGrid->removeWidget(m_posterLabel);
    m_topGrid->removeWidget(m_infoWidget);

    if (shouldVertical) {
        m_topGrid->addWidget(m_posterLabel, 0, 0, Qt::AlignHCenter);
        m_topGrid->addWidget(m_infoWidget, 1, 0);
        m_topGrid->setColumnStretch(0, 1);
        m_posterLabel->setFixedSize(140, 195);
    } else {
        m_topGrid->addWidget(m_posterLabel, 0, 0, Qt::AlignTop);
        m_topGrid->addWidget(m_infoWidget, 0, 1);
        m_topGrid->setColumnStretch(0, 0);
        m_topGrid->setColumnStretch(1, 1);
        m_posterLabel->setFixedSize(170, 235);
    }

    if (!m_movie.doubanId.isEmpty()) {
        loadPoster(m_movie.getPoster());
    }
}

void MovieDetailWidget::setMovie(const Movie& movie)
{
    m_movie = movie;
    m_userReview = UserReview(); // reset, will be filled async
    m_movie.userRating = 0;
    m_movie.isWished = false;
    m_movie.isWatched = false;
    updateUserSection();
    refreshPublicReviews({}); // clear old reviews, show empty state until data arrives

    // Async: request current user's review
    int uid = m_chatMgr->serverUserId();
    if (uid > 0) m_serverApi->getReview(uid, movie.doubanId);
    // Async: request all users' reviews
    m_serverApi->getMovieReviews(movie.doubanId);

    m_titleLabel->setText(movie.getName());
    m_originalTitleLabel->setText(movie.originalName != movie.getName()
                                  ? movie.originalName + (movie.alias.isEmpty() ? "" : " / " + movie.alias.split(" / ").first())
                                  : "");

    m_yearLabel->setText(movie.year);

    if (movie.duration > 0) {
        if (movie.episodes > 0)
            m_durationLabel->setText(QString("%1 分钟 · 共 %2 集").arg(movie.getDurationMinutes()).arg(movie.episodes));
        else if (movie.totalSeasons > 0)
            m_durationLabel->setText(QString("%1 分钟 · 共 %2 季").arg(movie.getDurationMinutes()).arg(movie.totalSeasons));
        else
            m_durationLabel->setText(QString("%1 分钟").arg(movie.getDurationMinutes()));
    } else if (movie.episodes > 0) {
        if (movie.totalSeasons > 0)
            m_durationLabel->setText(QString("共 %1 季 %2 集").arg(movie.totalSeasons).arg(movie.episodes));
        else
            m_durationLabel->setText(QString("共 %1 集").arg(movie.episodes));
    } else if (movie.totalSeasons > 0) {
        m_durationLabel->setText(QString("共 %1 季").arg(movie.totalSeasons));
    } else {
        m_durationLabel->setText("--");
    }

    m_genreLabel->setText(movie.getGenre().isEmpty() ? "--" : movie.getGenre());
    m_countryLabel->setText(movie.getCountry().isEmpty() ? "--" : movie.getCountry());

    QString lang;
    for (const auto& d : movie.langData) {
        if (d.lang == "Cn" && !d.language.isEmpty()) { lang = d.language; break; }
    }
    m_languageLabel->setText(lang.isEmpty() ? "--" : lang);

    m_dateReleasedLabel->setText(movie.dateReleased.isEmpty() ? "--" : movie.dateReleased);

    QStringList directors;
    for (const auto& d : movie.directors) directors << d.name;
    m_directorLabel->setText(directors.isEmpty() ? "--" : directors.join(" / "));

    QStringList writers;
    int wcnt = 0;
    for (const auto& w : movie.writers) {
        if (++wcnt > 4) break;
        writers << w.name;
    }
    m_writerLabel->setText(writers.isEmpty() ? "--" : writers.join(" / "));

    QStringList actors;
    int cnt = 0;
    for (const auto& a : movie.actors) {
        if (++cnt > 6) break;
        actors << a.name;
    }
    m_actorLabel->setText(actors.isEmpty() ? "--" : actors.join(" / "));

    if (!movie.alias.isEmpty()) {
        QStringList aliases = movie.alias.split(" / ");
        QStringList filtered;
        for (const auto& a : aliases) {
            if (a != movie.getName() && a != movie.originalName) filtered << a;
        }
        m_aliasLabel->setText(filtered.isEmpty() ? "--" : filtered.join(" / "));
    } else {
        m_aliasLabel->setText("--");
    }

    m_descLabel->setText(movie.getDescription().isEmpty() ? "暂无简介" : movie.getDescription());

    if (movie.doubanRating > 0)
        m_doubanRatingLabel->setText(QString::number(movie.doubanRating, 'f', 1));
    else
        m_doubanRatingLabel->setText("--");

    if (movie.imdbRating > 0)
        m_imdbRatingLabel->setText(QString::number(movie.imdbRating, 'f', 1));
    else
        m_imdbRatingLabel->setText("--");

    if (movie.rottenRating > 0)
        m_rottenRatingLabel->setText(QString("%1%").arg((int)movie.rottenRating));
    else
        m_rottenRatingLabel->setText("--");

    loadPoster(movie.getPoster());
    updateTopLayout();
    m_scrollArea->verticalScrollBar()->setValue(0);
}

void MovieDetailWidget::loadPoster(const QString& url)
{
    if (url.isEmpty()) {
        m_posterLabel->setText("🎬");
        m_posterLabel->setStyleSheet("background: #E8E8EC; border-radius: 10px; font-size: 28px; color: #BBB;");
        return;
    }

    ImageCache::instance().loadImage(url, this, [this](const QPixmap& pixmap) {
        if (!pixmap.isNull()) {
            QPixmap scaled = pixmap.scaled(m_posterLabel->size(),
                                           Qt::KeepAspectRatioByExpanding,
                                           Qt::SmoothTransformation);
            QPixmap rounded(m_posterLabel->size());
            rounded.fill(Qt::transparent);
            QPainter p(&rounded);
            p.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(rounded.rect(), 10, 10);
            p.setClipPath(path);
            int x = (rounded.width() - scaled.width()) / 2;
            int y = (rounded.height() - scaled.height()) / 2;
            p.drawPixmap(x, y, scaled);
            p.end();
            m_posterLabel->setPixmap(rounded);
        }
    });
}

void MovieDetailWidget::updateUserSection()
{
    m_wishBtn->setChecked(m_movie.isWished);
    m_wishBtn->setText(m_movie.isWished ? "♥ 想看" : "♡ 想看");
    m_watchedBtn->setChecked(m_movie.isWatched);
    m_watchedBtn->setText(m_movie.isWatched ? "✓ 看过" : "○ 看过");

    m_userRatingWidget->setRating(m_userReview.rating);

    if (m_userReview.rating > 0) {
        m_userRatingTextLabel->setText(QString("%1 分").arg(m_userReview.rating, 0, 'f', 1));
    } else {
        m_userRatingTextLabel->setText("尚未评分，点击\"写短评\"进行评价");
    }

    if (!m_userReview.content.isEmpty()) {
        m_userReviewLabel->setText("\"" + m_userReview.content + "\"");
        m_userReviewLabel->setVisible(true);
    } else {
        m_userReviewLabel->setVisible(false);
    }
}

static QWidget* createReviewCard(const UserReview& review)
{
    auto* card = new QWidget();
    card->setStyleSheet("background: #F9F9FB; border-radius: 8px;");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(6);

    auto* header = new QHBoxLayout();
    auto* nameLabel = new QLabel(review.username.isEmpty() ? "匿名" : review.username);
    nameLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #00B51D;");
    header->addWidget(nameLabel);

    if (review.rating > 0) {
        auto* starLabel = new QLabel();
        int fullStars = qRound(review.rating / 2.0);
        starLabel->setText(QString("★").repeated(fullStars) + QString("☆").repeated(5 - fullStars));
        starLabel->setStyleSheet("font-size: 11px; color: #F5A623;");
        header->addWidget(starLabel);
    }
    header->addStretch();

    auto* timeLabel = new QLabel(review.createTime.length() > 10
                                 ? review.createTime.left(10) : review.createTime);
    timeLabel->setStyleSheet("font-size: 11px; color: #BBB;");
    header->addWidget(timeLabel);
    layout->addLayout(header);

    auto* contentLabel = new QLabel(review.content);
    contentLabel->setWordWrap(true);
    contentLabel->setStyleSheet("font-size: 14px; color: #444; line-height: 1.6;");
    layout->addWidget(contentLabel);

    return card;
}

void MovieDetailWidget::refreshPublicReviews(const QList<UserReview>& reviews)
{
    QLayoutItem* item;
    while ((item = m_publicReviewsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    if (reviews.isEmpty()) {
        auto* emptyLabel = new QLabel("暂无评价，成为第一个评价的人吧！");
        emptyLabel->setStyleSheet("font-size: 14px; color: #BBB; padding: 20px;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        m_publicReviewsLayout->addWidget(emptyLabel);
        return;
    }

    for (const auto& review : reviews) {
        m_publicReviewsLayout->addWidget(createReviewCard(review));
    }
}

void MovieDetailWidget::onReviewReceived(const UserReview& review)
{
    if (review.doubanId != m_movie.doubanId) return;

    m_userReview = review;
    m_movie.userRating = review.rating;
    m_movie.isWished = review.isWished;
    m_movie.isWatched = review.isWatched;
    updateUserSection();
}

void MovieDetailWidget::onMovieReviewsReceived(const QString& doubanId, const QList<UserReview>& reviews)
{
    if (doubanId != m_movie.doubanId) return;
    refreshPublicReviews(reviews);
}

void MovieDetailWidget::onWriteReview()
{
    ReviewDialog dlg(m_movie, m_userReview, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_userReview.doubanId = m_movie.doubanId;
        m_userReview.movieName = m_movie.getName();
        m_userReview.rating = dlg.getRating();
        m_userReview.content = dlg.getReview();
        m_userReview.isWished = dlg.isWished();
        m_userReview.isWatched = dlg.isWatched();
        m_userReview.posterUrl = m_movie.getPoster();
        m_db->saveReview(m_userReview);

        m_movie.userRating = m_userReview.rating;
        m_movie.isWished = m_userReview.isWished;
        m_movie.isWatched = m_userReview.isWatched;
        updateUserSection();
        m_serverApi->getMovieReviews(m_movie.doubanId);
        emit reviewUpdated();
    }
}

void MovieDetailWidget::onToggleWish()
{
    m_movie.isWished = m_wishBtn->isChecked();
    m_userReview.isWished = m_movie.isWished;
    m_db->setWished(m_movie.doubanId, m_movie.getName(), m_movie.isWished, m_movie.getPoster());
    updateUserSection();
    emit reviewUpdated();
}

void MovieDetailWidget::onToggleWatched()
{
    m_movie.isWatched = m_watchedBtn->isChecked();
    m_userReview.isWatched = m_movie.isWatched;
    m_db->setWatched(m_movie.doubanId, m_movie.getName(), m_movie.isWatched, m_movie.getPoster());
    updateUserSection();
    emit reviewUpdated();
}
