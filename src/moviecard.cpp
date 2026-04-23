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
    setFixedSize(170, 285);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(R"(
        MovieCard {
            background: white;
            border-radius: 12px;
            border: 1px solid #ECECEC;
        }
    )");

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(16);
    shadow->setColor(QColor(0, 0, 0, 25));
    shadow->setOffset(0, 4);
    setGraphicsEffect(shadow);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 10);
    layout->setSpacing(4);

    m_posterLabel = new QLabel(this);
    m_posterLabel->setFixedSize(170, 190);
    m_posterLabel->setScaledContents(false);
    m_posterLabel->setAlignment(Qt::AlignCenter);
    m_posterLabel->setStyleSheet("background: #F0F0F0; border-radius: 12px 12px 0 0;");
    layout->addWidget(m_posterLabel);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_titleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #2D2D2D; padding: 0 10px;");
    m_titleLabel->setMaximumHeight(40);
    layout->addWidget(m_titleLabel);

    m_ratingLabel = new QLabel(this);
    m_ratingLabel->setAlignment(Qt::AlignCenter);
    m_ratingLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #FF6000;");
    layout->addWidget(m_ratingLabel);

    m_genreLabel = new QLabel(this);
    m_genreLabel->setAlignment(Qt::AlignCenter);
    m_genreLabel->setStyleSheet("font-size: 11px; color: #999; padding: 0 6px;");
    layout->addWidget(m_genreLabel);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 11px; color: #00B386; font-weight: bold;");
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
    } else {
        m_ratingLabel->setText("暂无评分");
        m_ratingLabel->setStyleSheet("font-size: 13px; color: #999;");
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
        m_posterLabel->setText("🎬 暂无海报");
        m_posterLabel->setStyleSheet("background: #F0F0F0; border-radius: 12px 12px 0 0; font-size: 13px; color: #BBB;");
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
            path.addRoundedRect(rounded.rect(), 12, 12);
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
            border-radius: 12px;
            border: 2px solid #00B386;
        }
    )");
    auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(graphicsEffect());
    if (shadow) {
        shadow->setBlurRadius(24);
        shadow->setColor(QColor(0, 179, 134, 70));
        shadow->setOffset(0, 6);
    }
    QFrame::enterEvent(event);
}

void MovieCard::leaveEvent(QEvent* event)
{
    setStyleSheet(R"(
        MovieCard {
            background: white;
            border-radius: 12px;
            border: 1px solid #ECECEC;
        }
    )");
    auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(graphicsEffect());
    if (shadow) {
        shadow->setBlurRadius(16);
        shadow->setColor(QColor(0, 0, 0, 25));
        shadow->setOffset(0, 4);
    }
    QFrame::leaveEvent(event);
}
