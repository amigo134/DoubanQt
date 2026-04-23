#include "moviecard.h"
#include "imagecache.h"
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>

MovieCard::MovieCard(const Movie& movie, QWidget* parent)
    : QFrame(parent)
{
    setFixedSize(170, 290);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(R"(
        MovieCard {
            background: white;
            border-radius: 10px;
            border: 1px solid #EEE;
        }
    )");

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 12));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 10);
    layout->setSpacing(4);

    auto* posterWrap = new QWidget();
    posterWrap->setFixedHeight(200);
    posterWrap->setStyleSheet("background: transparent;");

    auto* posterLayout = new QVBoxLayout(posterWrap);
    posterLayout->setContentsMargins(0, 0, 0, 0);
    posterLayout->setSpacing(0);

    m_posterLabel = new QLabel();
    m_posterLabel->setFixedHeight(200);
    m_posterLabel->setAlignment(Qt::AlignCenter);
    m_posterLabel->setStyleSheet("background: #E8E8EC; border-radius: 10px 10px 0 0;");

    posterLayout->addWidget(m_posterLabel);

    auto* badgeRow = new QHBoxLayout();
    badgeRow->setContentsMargins(8, -22, 0, 0);
    badgeRow->setSpacing(0);

    m_ratingBadge = new QWidget();
    m_ratingBadge->setFixedSize(40, 22);
    m_ratingBadge->setStyleSheet(R"(
        QWidget {
            background: rgba(0,0,0,0.60);
            border-radius: 4px;
        }
    )");
    auto* badgeLayout = new QHBoxLayout(m_ratingBadge);
    badgeLayout->setContentsMargins(6, 2, 6, 2);
    badgeLayout->setSpacing(2);
    m_ratingLabel = new QLabel();
    m_ratingLabel->setAlignment(Qt::AlignCenter);
    m_ratingLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #FFD700; background: transparent; border: none;");
    badgeLayout->addWidget(m_ratingLabel);

    badgeRow->addWidget(m_ratingBadge);
    badgeRow->addStretch();
    posterLayout->addLayout(badgeRow);

    layout->addWidget(posterWrap);

    m_titleLabel = new QLabel();
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_titleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #222; padding: 0 10px;");
    m_titleLabel->setMaximumHeight(38);
    layout->addWidget(m_titleLabel);

    m_genreLabel = new QLabel();
    m_genreLabel->setAlignment(Qt::AlignCenter);
    m_genreLabel->setStyleSheet("font-size: 11px; color: #999; padding: 0 8px;");
    layout->addWidget(m_genreLabel);

    m_statusLabel = new QLabel();
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 11px; color: #00B51D; font-weight: bold;");
    layout->addWidget(m_statusLabel);

    setMovie(movie);
}

void MovieCard::setMovie(const Movie& movie)
{
    m_movie = movie;

    QString title = movie.getName();
    if (title.length() > 14) {
        title = title.left(13) + "...";
    }
    m_titleLabel->setText(title);

    if (movie.doubanRating > 0) {
        m_ratingLabel->setText(QString::number(movie.doubanRating, 'f', 1));
        m_ratingBadge->setVisible(true);
    } else {
        m_ratingBadge->setVisible(false);
    }

    QString genre = movie.getGenre();
    if (genre.length() > 16) genre = genre.left(15) + "...";
    m_genreLabel->setText(genre);

    updateUserStatus();
    loadPoster(movie.getPoster());
}

void MovieCard::loadPoster(const QString& url)
{
    if (url.isEmpty()) {
        m_posterLabel->setText("🎬");
        m_posterLabel->setStyleSheet("background: #E8E8EC; border-radius: 10px 10px 0 0; font-size: 28px; color: #BBB;");
        return;
    }

    ImageCache::instance().loadImage(url, this, [this](const QPixmap& pixmap) {
        if (!pixmap.isNull()) {
            int w = m_posterLabel->width();
            int h = m_posterLabel->height();
            if (w <= 0 || h <= 0) { w = 170; h = 200; }
            QPixmap scaled = pixmap.scaled(w, h,
                                           Qt::KeepAspectRatioByExpanding,
                                           Qt::SmoothTransformation);
            QPixmap rounded(w, h);
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

void MovieCard::updateUserStatus()
{
    if (m_movie.userRating > 0) {
        m_statusLabel->setText(QString("我的评分: %1").arg(m_movie.userRating, 0, 'f', 1));
    } else if (m_movie.isWatched) {
        m_statusLabel->setText("✓ 看过");
    } else if (m_movie.isWished) {
        m_statusLabel->setText("★ 想看");
    } else {
        m_statusLabel->setText("");
    }
}

void MovieCard::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_movie);
    }
    QFrame::mousePressEvent(event);
}

void MovieCard::enterEvent(QEvent* event)
{
    setStyleSheet(R"(
        MovieCard {
            background: white;
            border-radius: 10px;
            border: 2px solid #00B51D;
        }
    )");
    auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(graphicsEffect());
    if (shadow) {
        shadow->setBlurRadius(16);
        shadow->setColor(QColor(0, 181, 29, 40));
        shadow->setOffset(0, 4);
    }
    QFrame::enterEvent(event);
}

void MovieCard::leaveEvent(QEvent* event)
{
    setStyleSheet(R"(
        MovieCard {
            background: white;
            border-radius: 10px;
            border: 1px solid #EEE;
        }
    )");
    auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(graphicsEffect());
    if (shadow) {
        shadow->setBlurRadius(12);
        shadow->setColor(QColor(0, 0, 0, 12));
        shadow->setOffset(0, 2);
    }
    QFrame::leaveEvent(event);
}
