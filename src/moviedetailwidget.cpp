#include "moviedetailwidget.h"
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

MovieDetailWidget::MovieDetailWidget(DatabaseManager* db, QWidget* parent)
    : QWidget(parent)
    , m_db(db)
{
    buildUI();
}

void MovieDetailWidget::buildUI()
{
    setStyleSheet(R"(
        MovieDetailWidget {
            background: #F5F6F8;
        }
        QLabel#sectionTitle {
            font-size: 16px;
            font-weight: bold;
            color: #2D2D2D;
            border-left: 4px solid #00B386;
            padding-left: 10px;
            margin-top: 8px;
        }
        QLabel#metaLabel {
            font-size: 13px;
            color: #555;
            line-height: 1.6;
        }
        QPushButton#backBtn {
            background: transparent;
            color: #00B386;
            border: none;
            font-size: 14px;
            font-weight: bold;
            text-align: left;
            padding: 6px 0;
        }
        QPushButton#backBtn:hover {
            color: #009A73;
        }
        QPushButton#wishBtn {
            background: #FFF8ED;
            color: #FF8C00;
            border: 1px solid #FFD580;
            border-radius: 20px;
            padding: 9px 22px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton#wishBtn:checked, QPushButton#wishBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #FFA030, stop:1 #FF8C00);
            color: white;
            border-color: #FF8C00;
        }
        QPushButton#watchedBtn {
            background: #E8FFF5;
            color: #00B386;
            border: 1px solid #7FDFC4;
            border-radius: 20px;
            padding: 9px 22px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton#watchedBtn:checked, QPushButton#watchedBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #00C49A, stop:1 #009A73);
            color: white;
            border-color: #00B386;
        }
        QPushButton#reviewBtn {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #00C49A, stop:1 #009A73);
            color: white;
            border: none;
            border-radius: 20px;
            padding: 9px 28px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton#reviewBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #00D4AA, stop:1 #00AA83);
        }
        QPushButton#reviewBtn:pressed {
            background: #008A63;
        }
        QFrame#ratingCard {
            background: white;
            border-radius: 12px;
            border: 1px solid #ECECEC;
        }
    )");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* navBar = new QWidget();
    navBar->setStyleSheet("background: white; border-bottom: 1px solid #ECECEC;");
    navBar->setFixedHeight(54);
    auto* navLayout = new QHBoxLayout(navBar);
    navLayout->setContentsMargins(20, 0, 20, 0);

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
        QScrollBar:vertical { width: 8px; background: transparent; }
        QScrollBar::handle:vertical { background: rgba(0,0,0,0.15); border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: rgba(0,0,0,0.25); }
    )");

    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background: #F5F6F8;");
    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);

    auto* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(28, 28, 28, 28);
    contentLayout->setSpacing(18);

    auto* topWidget = new QWidget();
    topWidget->setStyleSheet("background: white; border-radius: 14px; border: 1px solid #ECECEC;");
    auto* topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(24, 24, 24, 24);
    topLayout->setSpacing(28);

    m_posterLabel = new QLabel();
    m_posterLabel->setFixedSize(170, 235);
    m_posterLabel->setAlignment(Qt::AlignCenter);
    m_posterLabel->setStyleSheet("background: #F0F0F0; border-radius: 10px;");
    topLayout->addWidget(m_posterLabel, 0, Qt::AlignTop);

    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(8);

    m_titleLabel = new QLabel();
    m_titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #1A1A1A;");
    m_titleLabel->setWordWrap(true);
    infoLayout->addWidget(m_titleLabel);

    m_originalTitleLabel = new QLabel();
    m_originalTitleLabel->setStyleSheet("font-size: 14px; color: #888;");
    infoLayout->addWidget(m_originalTitleLabel);

    auto* metaFrame = new QFrame();
    metaFrame->setStyleSheet("background: #F8F9FA; border-radius: 10px;");
    auto* metaLayout = new QGridLayout(metaFrame);
    metaLayout->setContentsMargins(14, 14, 14, 14);
    metaLayout->setVerticalSpacing(8);
    metaLayout->setHorizontalSpacing(14);

    auto makeMetaLabel = [](const QString& text) {
        auto* l = new QLabel(text);
        l->setObjectName("metaLabel");
        return l;
    };

    m_yearLabel = makeMetaLabel("");
    m_durationLabel = makeMetaLabel("");
    m_genreLabel = makeMetaLabel("");
    m_countryLabel = makeMetaLabel("");
    m_directorLabel = makeMetaLabel("");
    m_directorLabel->setWordWrap(true);
    m_actorLabel = makeMetaLabel("");
    m_actorLabel->setWordWrap(true);

    metaLayout->addWidget(new QLabel("<b>年份</b>"), 0, 0);
    metaLayout->addWidget(m_yearLabel, 0, 1);
    metaLayout->addWidget(new QLabel("<b>时长</b>"), 1, 0);
    metaLayout->addWidget(m_durationLabel, 1, 1);
    metaLayout->addWidget(new QLabel("<b>类型</b>"), 2, 0);
    metaLayout->addWidget(m_genreLabel, 2, 1);
    metaLayout->addWidget(new QLabel("<b>地区</b>"), 3, 0);
    metaLayout->addWidget(m_countryLabel, 3, 1);
    metaLayout->addWidget(new QLabel("<b>导演</b>"), 4, 0);
    metaLayout->addWidget(m_directorLabel, 4, 1);
    metaLayout->addWidget(new QLabel("<b>主演</b>"), 5, 0, 1, 1, Qt::AlignTop);
    metaLayout->addWidget(m_actorLabel, 5, 1);
    metaLayout->setColumnStretch(1, 1);
    infoLayout->addWidget(metaFrame);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    m_wishBtn = new QPushButton("♡ 想看");
    m_wishBtn->setObjectName("wishBtn");
    m_wishBtn->setCheckable(true);
    m_watchedBtn = new QPushButton("✓ 看过");
    m_watchedBtn->setObjectName("watchedBtn");
    m_watchedBtn->setCheckable(true);
    m_reviewBtn = new QPushButton("✏ 写短评");
    m_reviewBtn->setObjectName("reviewBtn");

    connect(m_wishBtn, &QPushButton::clicked, this, &MovieDetailWidget::onToggleWish);
    connect(m_watchedBtn, &QPushButton::clicked, this, &MovieDetailWidget::onToggleWatched);
    connect(m_reviewBtn, &QPushButton::clicked, this, &MovieDetailWidget::onWriteReview);

    btnLayout->addWidget(m_wishBtn);
    btnLayout->addWidget(m_watchedBtn);
    btnLayout->addWidget(m_reviewBtn);
    btnLayout->addStretch();
    infoLayout->addLayout(btnLayout);
    infoLayout->addStretch();

    topLayout->addLayout(infoLayout, 1);
    contentLayout->addWidget(topWidget);

    auto* ratingCard = new QFrame();
    ratingCard->setObjectName("ratingCard");
    auto* ratingLayout = new QHBoxLayout(ratingCard);
    ratingLayout->setContentsMargins(24, 20, 24, 20);
    ratingLayout->setSpacing(0);

    auto makePlatformRating = [&](const QString& platform, QLabel*& ratingLabel, const QString& color) {
        auto* col = new QVBoxLayout();
        col->setSpacing(4);
        auto* platformLabel = new QLabel(platform);
        platformLabel->setAlignment(Qt::AlignCenter);
        platformLabel->setStyleSheet("font-size: 12px; color: #888;");
        ratingLabel = new QLabel("--");
        ratingLabel->setAlignment(Qt::AlignCenter);
        ratingLabel->setStyleSheet(QString("font-size: 28px; font-weight: bold; color: %1;").arg(color));
        col->addWidget(ratingLabel);
        col->addWidget(platformLabel);
        return col;
    };

    ratingLayout->addLayout(makePlatformRating("豆瓣", m_doubanRatingLabel, "#FF6000"));
    auto* divider1 = new QFrame();
    divider1->setFrameShape(QFrame::VLine);
    divider1->setStyleSheet("color: #ECECEC;");
    ratingLayout->addWidget(divider1);
    ratingLayout->addLayout(makePlatformRating("IMDb", m_imdbRatingLabel, "#F5C518"));
    auto* divider2 = new QFrame();
    divider2->setFrameShape(QFrame::VLine);
    divider2->setStyleSheet("color: #ECECEC;");
    ratingLayout->addWidget(divider2);
    ratingLayout->addLayout(makePlatformRating("烂番茄", m_rottenRatingLabel, "#FA320A"));

    contentLayout->addWidget(ratingCard);

    auto* descTitle = new QLabel("剧情简介");
    descTitle->setObjectName("sectionTitle");
    contentLayout->addWidget(descTitle);

    m_descLabel = new QLabel();
    m_descLabel->setWordWrap(true);
    m_descLabel->setStyleSheet("font-size: 14px; color: #555; line-height: 1.8; background: white; border-radius: 12px; padding: 18px; border: 1px solid #ECECEC;");
    m_descLabel->setTextFormat(Qt::PlainText);
    contentLayout->addWidget(m_descLabel);

    auto* mySection = new QLabel("我的评价");
    mySection->setObjectName("sectionTitle");
    contentLayout->addWidget(mySection);

    auto* userRatingCard = new QFrame();
    userRatingCard->setObjectName("ratingCard");
    auto* userRatingLayout = new QVBoxLayout(userRatingCard);
    userRatingLayout->setContentsMargins(24, 20, 24, 20);
    userRatingLayout->setSpacing(10);

    auto* ratingRow = new QHBoxLayout();
    m_userRatingWidget = new RatingWidget(this);
    m_userRatingWidget->setStarSize(28);
    m_userRatingWidget->setReadOnly(true);

    m_userRatingTextLabel = new QLabel("尚未评分");
    m_userRatingTextLabel->setStyleSheet("font-size: 14px; color: #666; margin-left: 12px; font-weight: bold;");
    ratingRow->addWidget(m_userRatingWidget);
    ratingRow->addWidget(m_userRatingTextLabel);
    ratingRow->addStretch();
    userRatingLayout->addLayout(ratingRow);

    m_userReviewLabel = new QLabel();
    m_userReviewLabel->setWordWrap(true);
    m_userReviewLabel->setStyleSheet("font-size: 14px; color: #555; border-left: 4px solid #00B386; padding-left: 12px; margin-top: 4px;");
    m_userReviewLabel->setVisible(false);
    userRatingLayout->addWidget(m_userReviewLabel);

    contentLayout->addWidget(userRatingCard);
    contentLayout->addStretch();
}

void MovieDetailWidget::setMovie(const Movie& movie)
{
    m_movie = movie;
    m_userReview = m_db->getReview(movie.doubanId);
    m_movie.userRating = m_userReview.rating;
    m_movie.isWished = m_userReview.isWished;
    m_movie.isWatched = m_userReview.isWatched;

    m_titleLabel->setText(movie.getName());
    m_originalTitleLabel->setText(movie.originalName != movie.getName()
                                  ? movie.originalName + (movie.alias.isEmpty() ? "" : " / " + movie.alias.split(" / ").first())
                                  : "");

    m_yearLabel->setText(movie.year);

    if (movie.duration > 0) {
        m_durationLabel->setText(QString("%1 分钟").arg(movie.getDurationMinutes()));
    } else if (movie.episodes > 0) {
        m_durationLabel->setText(QString("共 %1 集").arg(movie.episodes));
    } else {
        m_durationLabel->setText("--");
    }

    m_genreLabel->setText(movie.getGenre().isEmpty() ? "--" : movie.getGenre());
    m_countryLabel->setText(movie.getCountry().isEmpty() ? "--" : movie.getCountry());

    QStringList directors;
    for (const auto& d : movie.directors) directors << d.name;
    m_directorLabel->setText(directors.isEmpty() ? "--" : directors.join(" / "));

    QStringList actors;
    int cnt = 0;
    for (const auto& a : movie.actors) {
        if (++cnt > 6) break;
        actors << a.name;
    }
    m_actorLabel->setText(actors.isEmpty() ? "--" : actors.join(" / "));

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
    updateUserSection();
    m_scrollArea->verticalScrollBar()->setValue(0);
}

void MovieDetailWidget::loadPoster(const QString& url)
{
    if (url.isEmpty()) {
        m_posterLabel->setText("🎬 暂无海报");
        m_posterLabel->setStyleSheet("background: #F0F0F0; border-radius: 10px; font-size: 14px; color: #BBB;");
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
        m_db->saveReview(m_userReview);

        m_movie.userRating = m_userReview.rating;
        m_movie.isWished = m_userReview.isWished;
        m_movie.isWatched = m_userReview.isWatched;
        updateUserSection();
        emit reviewUpdated();
    }
}

void MovieDetailWidget::onToggleWish()
{
    m_movie.isWished = m_wishBtn->isChecked();
    m_userReview.isWished = m_movie.isWished;
    m_db->setWished(m_movie.doubanId, m_movie.getName(), m_movie.isWished);
    updateUserSection();
    emit reviewUpdated();
}

void MovieDetailWidget::onToggleWatched()
{
    m_movie.isWatched = m_watchedBtn->isChecked();
    m_userReview.isWatched = m_movie.isWatched;
    m_db->setWatched(m_movie.doubanId, m_movie.getName(), m_movie.isWatched);
    updateUserSection();
    emit reviewUpdated();
}
