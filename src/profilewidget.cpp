#include "profilewidget.h"
#include "profileeditdialog.h"
#include "imagecache.h"
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QScrollBar>
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QPainterPath>

ProfileWidget::ProfileWidget(DatabaseManager* db, QWidget* parent)
    : QWidget(parent)
    , m_db(db)
    , m_resizeTimer(new QTimer(this))
{
    m_resizeTimer->setSingleShot(true);
    m_resizeTimer->setInterval(200);
    connect(m_resizeTimer, &QTimer::timeout, this, [this]() {
        rebuildWishRows();
        rebuildWatchedRows();
    });

    buildUI();
}

void ProfileWidget::buildUI()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->verticalScrollBar()->setStyleSheet(R"(
        QScrollBar:vertical { width: 6px; background: transparent; }
        QScrollBar::handle:vertical { background: rgba(0,0,0,0.12); border-radius: 3px; }
    )");

    auto* content = new QWidget();
    content->setStyleSheet("background: #F5F6F8;");
    scroll->setWidget(content);

    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(18);

    m_profileCard = new QFrame();
    m_profileCard->setMaximumWidth(600);
    m_profileCard->setStyleSheet(R"(
        QFrame { background: white; border-radius: 12px; border: 1px solid #EEE; }
    )");
    auto* profileLayout = new QHBoxLayout(m_profileCard);
    profileLayout->setContentsMargins(28, 24, 28, 24);
    profileLayout->setSpacing(20);

    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(72, 72);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setCursor(Qt::PointingHandCursor);
    m_avatarLabel->installEventFilter(this);
    m_avatarLabel->setStyleSheet(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00B51D, stop:1 #00D4AA);
        border-radius: 36px;
        font-size: 28px;
        color: white;
        font-weight: bold;
    )");
    m_avatarLabel->setText("U");

    auto* infoCol = new QVBoxLayout();
    infoCol->setSpacing(4);

    m_nameLabel = new QLabel("影迷");
    m_nameLabel->setCursor(Qt::PointingHandCursor);
    m_nameLabel->installEventFilter(this);
    m_nameLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #222;");

    m_bioLabel = new QLabel("记录每一部看过的电影");
    m_bioLabel->setCursor(Qt::PointingHandCursor);
    m_bioLabel->installEventFilter(this);
    m_bioLabel->setStyleSheet("font-size: 13px; color: #999;");

    infoCol->addWidget(m_nameLabel);
    infoCol->addWidget(m_bioLabel);

    auto makeStatBlock = [](const QString& label, QLabel*& valueLabel, const QString& color) {
        auto* col = new QVBoxLayout();
        col->setSpacing(2);
        valueLabel = new QLabel("0");
        valueLabel->setAlignment(Qt::AlignCenter);
        valueLabel->setStyleSheet(QString("font-size: 24px; font-weight: bold; color: %1;").arg(color));
        auto* lbl = new QLabel(label);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("font-size: 12px; color: #999;");
        col->addWidget(valueLabel);
        col->addWidget(lbl);
        auto* w = new QWidget();
        w->setLayout(col);
        return w;
    };

    auto* statRow = new QHBoxLayout();
    statRow->setSpacing(0);
    statRow->addWidget(makeStatBlock("看过", m_statWatched, "#00B51D"));
    statRow->addWidget(makeStatBlock("想看", m_statWished, "#F5A623"));
    statRow->addWidget(makeStatBlock("评价", m_statReviews, "#5B7FFF"));

    profileLayout->addWidget(m_avatarLabel);
    profileLayout->addLayout(infoCol);
    profileLayout->addStretch();
    profileLayout->addLayout(statRow);

    auto* profileWrap = new QHBoxLayout();
    profileWrap->addStretch();
    profileWrap->addWidget(m_profileCard);
    profileWrap->addStretch();
    root->addLayout(profileWrap);

    auto* wishTitle = new QLabel("想看");
    wishTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #333; border-left: 3px solid #F5A623; padding-left: 8px;");
    root->addWidget(wishTitle);

    m_wishWrap = new QWidget();
    m_wishWrap->setStyleSheet("background: white; border-radius: 10px;");
    m_wishOuterLayout = new QVBoxLayout(m_wishWrap);
    m_wishOuterLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    m_wishOuterLayout->setSpacing(SPACING);

    m_wishEmptyLabel = new QLabel("还没有想看的电影");
    m_wishEmptyLabel->setAlignment(Qt::AlignCenter);
    m_wishEmptyLabel->setStyleSheet("font-size: 13px; color: #BBB; padding: 36px;");
    m_wishOuterLayout->addWidget(m_wishEmptyLabel);

    root->addWidget(m_wishWrap);

    auto* watchedTitle = new QLabel("看过");
    watchedTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #333; border-left: 3px solid #00B51D; padding-left: 8px;");
    root->addWidget(watchedTitle);

    m_watchedWrap = new QWidget();
    m_watchedWrap->setStyleSheet("background: white; border-radius: 10px;");
    m_watchedOuterLayout = new QVBoxLayout(m_watchedWrap);
    m_watchedOuterLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    m_watchedOuterLayout->setSpacing(SPACING);

    m_watchedEmptyLabel = new QLabel("还没有看过的电影");
    m_watchedEmptyLabel->setAlignment(Qt::AlignCenter);
    m_watchedEmptyLabel->setStyleSheet("font-size: 13px; color: #BBB; padding: 36px;");
    m_watchedOuterLayout->addWidget(m_watchedEmptyLabel);

    root->addWidget(m_watchedWrap);

    auto* reviewTitle = new QLabel("我的评价");
    reviewTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #333; border-left: 3px solid #5B7FFF; padding-left: 8px;");
    root->addWidget(reviewTitle);

    m_reviewWrap = new QWidget();
    m_reviewWrap->setStyleSheet("background: white; border-radius: 10px;");
    m_reviewOuterLayout = new QVBoxLayout(m_reviewWrap);
    m_reviewOuterLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    m_reviewOuterLayout->setSpacing(SPACING);

    m_reviewEmptyLabel = new QLabel("还没有写过评价");
    m_reviewEmptyLabel->setAlignment(Qt::AlignCenter);
    m_reviewEmptyLabel->setStyleSheet("font-size: 13px; color: #BBB; padding: 36px;");
    m_reviewOuterLayout->addWidget(m_reviewEmptyLabel);

    root->addWidget(m_reviewWrap);

    auto* logoutBtn = new QPushButton("退出登录");
    logoutBtn->setFixedHeight(40);
    logoutBtn->setMaximumWidth(200);
    logoutBtn->setStyleSheet(R"(
        QPushButton {
            background: white;
            color: #e74c3c;
            border: 1px solid #e74c3c;
            border-radius: 8px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover { background: #e74c3c; color: white; }
    )");
    connect(logoutBtn, &QPushButton::clicked, this, &ProfileWidget::logoutRequested);

    auto* logoutWrap = new QHBoxLayout();
    logoutWrap->addStretch();
    logoutWrap->addWidget(logoutBtn);
    logoutWrap->addStretch();
    root->addLayout(logoutWrap);

    root->addStretch();

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);
}

void ProfileWidget::refresh()
{
    loadProfile();
    loadData();
}

void ProfileWidget::loadData()
{
    for (auto* card : m_wishCards) { m_wishOuterLayout->removeWidget(card); delete card; }
    m_wishCards.clear();
    for (auto* card : m_watchedCards) { m_watchedOuterLayout->removeWidget(card); delete card; }
    m_watchedCards.clear();
    for (auto* card : m_reviewCards) { m_reviewOuterLayout->removeWidget(card); delete card; }
    m_reviewCards.clear();

    m_wishData = m_db->getWishList();
    m_watchedData = m_db->getWatchedList();
    m_reviewData = m_db->getAllReviews();

    int watchedCount = m_watchedData.size();
    int wishedCount = m_wishData.size();
    int reviewCount = 0;
    for (const auto& r : m_reviewData) {
        if (!r.content.isEmpty() || r.rating > 0) reviewCount++;
    }

    m_statWatched->setText(QString::number(watchedCount));
    m_statWished->setText(QString::number(wishedCount));
    m_statReviews->setText(QString::number(reviewCount));

    auto loadPoster = [](QLabel* posterLabel, const QString& url) {
        if (!url.isEmpty()) {
            ImageCache::instance().loadImage(url, posterLabel, [posterLabel](const QPixmap& pixmap) {
                if (!pixmap.isNull()) {
                    QPixmap scaled = pixmap.scaled(posterLabel->size(),
                        Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                    QPixmap rounded(posterLabel->size());
                    rounded.fill(Qt::transparent);
                    QPainter p(&rounded);
                    p.setRenderHint(QPainter::Antialiasing);
                    QPainterPath path;
                    path.addRoundedRect(rounded.rect(), 6, 6);
                    p.setClipPath(path);
                    int x = (rounded.width() - scaled.width()) / 2;
                    int y = (rounded.height() - scaled.height()) / 2;
                    p.drawPixmap(x, y, scaled);
                    p.end();
                    posterLabel->setPixmap(rounded);
                }
            });
        }
    };

    for (int i = 0; i < m_wishData.size(); ++i) {
        const auto& item = m_wishData[i];
        auto* card = new QFrame(m_wishWrap);
        card->setFixedSize(CARD_W, CARD_H);
        card->setStyleSheet(R"(
            QFrame { background: white; border-radius: 8px; border: 1px solid #E8E8E8; }
            QFrame:hover { border-color: #F5A623; background: #FFFBF0; }
        )");
        card->setCursor(Qt::PointingHandCursor);
        card->installEventFilter(this);

        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(6, 6, 6, 6);
        cl->setSpacing(3);

        auto* posterLabel = new QLabel();
        posterLabel->setFixedSize(98, 120);
        posterLabel->setAlignment(Qt::AlignCenter);
        posterLabel->setStyleSheet("background: #E8E8EC; border-radius: 6px; font-size: 18px; color: #BBB;");
        posterLabel->setText("🎬");
        loadPoster(posterLabel, item.posterUrl);
        cl->addWidget(posterLabel);

        auto* nm = new QLabel(item.movieName.left(5) + (item.movieName.length() > 5 ? "..." : ""));
        nm->setStyleSheet("font-size: 11px; color: #333; font-weight: bold;");
        cl->addWidget(nm);

        m_wishCards.append(card);
    }

    for (int i = 0; i < m_watchedData.size(); ++i) {
        const auto& item = m_watchedData[i];
        auto* card = new QFrame(m_watchedWrap);
        card->setFixedSize(CARD_W, CARD_H);
        card->setStyleSheet(R"(
            QFrame { background: white; border-radius: 8px; border: 1px solid #E8E8E8; }
            QFrame:hover { border-color: #00B51D; background: #F0FFF0; }
        )");
        card->setCursor(Qt::PointingHandCursor);
        card->installEventFilter(this);

        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(6, 6, 6, 6);
        cl->setSpacing(3);

        auto* posterLabel = new QLabel();
        posterLabel->setFixedSize(98, 120);
        posterLabel->setAlignment(Qt::AlignCenter);
        posterLabel->setStyleSheet("background: #E8E8EC; border-radius: 6px; font-size: 18px; color: #BBB;");
        posterLabel->setText("🎬");
        loadPoster(posterLabel, item.posterUrl);
        cl->addWidget(posterLabel);

        auto* nm = new QLabel(item.movieName.left(5) + (item.movieName.length() > 5 ? "..." : ""));
        nm->setStyleSheet("font-size: 11px; color: #333; font-weight: bold;");
        cl->addWidget(nm);

        if (item.rating > 0) {
            auto* rt = new QLabel(QString("⭐%1").arg(item.rating, 0, 'f', 1));
            rt->setStyleSheet("font-size: 10px; color: #FF6000; font-weight: bold;");
            cl->addWidget(rt);
        }

        m_watchedCards.append(card);
    }

    rebuildWishRows();
    rebuildWatchedRows();
    rebuildReviewRows();
}

int ProfileWidget::calcCols(int cardW) const
{
    int w = width() - 2 * 24 - 2 * MARGIN;
    if (w <= 0) w = 800;
    int cols = w / (cardW + SPACING);
    int rem = w - cols * (cardW + SPACING) + SPACING;
    if (rem >= cardW) cols++;
    return qMax(1, cols);
}

void ProfileWidget::rebuildWishRows()
{
    if (m_wishCards.isEmpty()) return;

    while (QLayoutItem* item = m_wishOuterLayout->takeAt(0)) {
        QLayout* sub = item->layout();
        if (sub) { while (QLayoutItem* si = sub->takeAt(0)) delete si; }
        delete item;
    }

    if (m_wishData.isEmpty()) {
        m_wishEmptyLabel->setVisible(true);
        m_wishOuterLayout->addWidget(m_wishEmptyLabel);
        return;
    }
    m_wishEmptyLabel->setVisible(false);

    int cols = calcCols(CARD_W);
    m_wishCols = cols;
    QHBoxLayout* row = nullptr;
    for (int i = 0; i < m_wishCards.size(); ++i) {
        if (i % cols == 0) {
            row = new QHBoxLayout();
            row->setSpacing(SPACING);
            row->setAlignment(Qt::AlignLeft);
            m_wishOuterLayout->addLayout(row);
        }
        row->addWidget(m_wishCards[i]);
    }
}

void ProfileWidget::rebuildWatchedRows()
{
    if (m_watchedCards.isEmpty()) return;

    while (QLayoutItem* item = m_watchedOuterLayout->takeAt(0)) {
        QLayout* sub = item->layout();
        if (sub) { while (QLayoutItem* si = sub->takeAt(0)) delete si; }
        delete item;
    }

    if (m_watchedData.isEmpty()) {
        m_watchedEmptyLabel->setVisible(true);
        m_watchedOuterLayout->addWidget(m_watchedEmptyLabel);
        return;
    }
    m_watchedEmptyLabel->setVisible(false);

    int cols = calcCols(CARD_W);
    m_watchedCols = cols;
    QHBoxLayout* row = nullptr;
    for (int i = 0; i < m_watchedCards.size(); ++i) {
        if (i % cols == 0) {
            row = new QHBoxLayout();
            row->setSpacing(SPACING);
            row->setAlignment(Qt::AlignLeft);
            m_watchedOuterLayout->addLayout(row);
        }
        row->addWidget(m_watchedCards[i]);
    }
}

void ProfileWidget::rebuildReviewRows()
{
    for (auto* card : m_reviewCards) {
        m_reviewOuterLayout->removeWidget(card);
        delete card;
    }
    m_reviewCards.clear();
    m_reviewFiltered.clear();

    if (m_reviewData.isEmpty()) {
        m_reviewEmptyLabel->setVisible(true);
        m_reviewOuterLayout->addWidget(m_reviewEmptyLabel);
        return;
    }
    m_reviewEmptyLabel->setVisible(false);

    for (const auto& review : m_reviewData) {
        if (review.content.isEmpty() && review.rating <= 0) continue;
        m_reviewFiltered.append(review);

        auto* card = new QFrame(m_reviewWrap);
        card->setStyleSheet(R"(
            QFrame { background: #FAFAFA; border-radius: 8px; border: 1px solid #EEE; }
            QFrame:hover { border-color: #5B7FFF; background: #F8F9FF; }
        )");
        card->setCursor(Qt::PointingHandCursor);
        card->installEventFilter(this);

        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(14, 12, 14, 12);
        cl->setSpacing(6);

        auto* topRow = new QHBoxLayout();
        auto* nameLbl = new QLabel(review.movieName.isEmpty() ? "未知电影" : review.movieName);
        nameLbl->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
        topRow->addWidget(nameLbl);

        if (review.rating > 0) {
            auto* ratingLbl = new QLabel(QString("⭐%1").arg(review.rating, 0, 'f', 1));
            ratingLbl->setStyleSheet("font-size: 13px; color: #FF6000; font-weight: bold;");
            topRow->addStretch();
            topRow->addWidget(ratingLbl);
        }

        cl->addLayout(topRow);

        if (!review.content.isEmpty()) {
            auto* contentLbl = new QLabel(review.content);
            contentLbl->setWordWrap(true);
            contentLbl->setStyleSheet("font-size: 13px; color: #666; line-height: 1.5;");
            cl->addWidget(contentLbl);
        }

        if (!review.createTime.isEmpty()) {
            auto* timeLbl = new QLabel(review.createTime.left(10));
            timeLbl->setStyleSheet("font-size: 11px; color: #BBB;");
            cl->addWidget(timeLbl);
        }

        m_reviewCards.append(card);
        m_reviewOuterLayout->addWidget(card);
    }
}

void ProfileWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_resizeTimer->start();
}

bool ProfileWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        if (watched == m_avatarLabel) {
            QString path = QFileDialog::getOpenFileName(this, "选择头像", QString(),
                "图片 (*.png *.jpg *.jpeg *.bmp *.gif)");
            if (!path.isEmpty()) {
                QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
                QDir().mkpath(dataPath);
                QString dest = dataPath + "/avatar" + QFileInfo(path).suffix().prepend('.');
                QFile::remove(dest);
                QFile::copy(path, dest);
                m_db->saveAvatarPath(dest);
                loadAvatar(dest);
            }
            return true;
        }
        if (watched == m_nameLabel || watched == m_bioLabel) {
            ProfileEditDialog dlg(m_nameLabel->text(), m_bioLabel->text(), this);
            if (dlg.exec() == QDialog::Accepted) {
                saveProfile(dlg.getName(), dlg.getBio());
            }
            return true;
        }
        int idx = m_wishCards.indexOf(qobject_cast<QFrame*>(watched));
        if (idx >= 0 && idx < m_wishData.size()) {
            Movie m;
            m.doubanId = m_wishData[idx].doubanId;
            m.originalName = m_wishData[idx].movieName;
            emit movieClicked(m);
            return true;
        }
        idx = m_watchedCards.indexOf(qobject_cast<QFrame*>(watched));
        if (idx >= 0 && idx < m_watchedData.size()) {
            Movie m;
            m.doubanId = m_watchedData[idx].doubanId;
            m.originalName = m_watchedData[idx].movieName;
            emit movieClicked(m);
            return true;
        }
        idx = m_reviewCards.indexOf(qobject_cast<QFrame*>(watched));
        if (idx >= 0 && idx < m_reviewFiltered.size()) {
            Movie m;
            m.doubanId = m_reviewFiltered[idx].doubanId;
            m.originalName = m_reviewFiltered[idx].movieName;
            emit movieClicked(m);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ProfileWidget::loadProfile()
{
    QString name = m_db->getProfileName();
    QString bio = m_db->getProfileBio();
    qDebug() << "loadProfile: name=" << name << "bio=" << bio << "userId=" << m_db->currentUserId();
    m_nameLabel->setText(name.isEmpty() ? "影迷" : name);
    m_bioLabel->setText(bio.isEmpty() ? "记录每一部看过的电影" : bio);

    QString avatarPath = m_db->getAvatarPath();
    if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
        loadAvatar(avatarPath);
    } else {
        m_avatarLabel->setPixmap(QPixmap());
        m_avatarLabel->setText(name.isEmpty() ? "U" : name.left(1).toUpper());
        m_avatarLabel->setStyleSheet(R"(
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00B51D, stop:1 #00D4AA);
            border-radius: 36px;
            font-size: 28px;
            color: white;
            font-weight: bold;
        )");
    }
}

void ProfileWidget::loadAvatar(const QString& path)
{
    QPixmap pix(path);
    if (pix.isNull()) return;
    int size = m_avatarLabel->width();
    QPixmap scaled = pix.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPixmap rounded(size, size);
    rounded.fill(Qt::transparent);
    QPainter p(&rounded);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addEllipse(0, 0, size, size);
    p.setClipPath(clip);
    int x = (size - scaled.width()) / 2;
    int y = (size - scaled.height()) / 2;
    p.drawPixmap(x, y, scaled);
    p.end();
    m_avatarLabel->setPixmap(rounded);
    m_avatarLabel->setText(QString());
    m_avatarLabel->setStyleSheet("background: transparent; border-radius: 36px;");
}

void ProfileWidget::saveProfile(const QString& name, const QString& bio)
{
    m_db->saveProfile(name, bio);
    m_nameLabel->setText(name);
    m_bioLabel->setText(bio.isEmpty() ? "记录每一部看过的电影" : bio);
    QString avatarPath = m_db->getAvatarPath();
    if (avatarPath.isEmpty() || !QFile::exists(avatarPath)) {
        m_avatarLabel->setPixmap(QPixmap());
        m_avatarLabel->setText(name.left(1).toUpper());
        m_avatarLabel->setStyleSheet(R"(
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00B51D, stop:1 #00D4AA);
            border-radius: 36px;
            font-size: 28px;
            color: white;
            font-weight: bold;
        )");
    }
}
