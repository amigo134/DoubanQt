#include "homewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QScrollBar>
#include <QFrame>
#include <QLabel>

const QStringList HomeWidget::HOT_SEARCHES = {
    "流浪地球", "哪吒", "长津湖", "封神", "满江红",
    "肖申克的救赎", "阿甘正传", "泰坦尼克号", "盗梦空间",
    "星际穿越", "权力的游戏", "破产姐妹", "老友记"
};

HomeWidget::HomeWidget(DatabaseManager* db, QWidget* parent)
    : QWidget(parent)
    , m_db(db)
{
    buildUI();

    m_resizeTimer = new QTimer(this);
    m_resizeTimer->setSingleShot(true);
    m_resizeTimer->setInterval(200);
    connect(m_resizeTimer, &QTimer::timeout, this, [this]() {
        rearrangeHotSearch();
        rearrangeMyList();
    });
}

void HomeWidget::buildUI()
{
    setStyleSheet("background: #F5F6F8;");

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->verticalScrollBar()->setStyleSheet(R"(
        QScrollBar:vertical { width: 8px; background: transparent; }
        QScrollBar::handle:vertical { background: rgba(0,0,0,0.15); border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: rgba(0,0,0,0.25); }
    )");

    auto* content = new QWidget();
    content->setStyleSheet("background: #F5F6F8;");
    scrollArea->setWidget(content);

    auto* mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(20);

    auto* banner = new QFrame();
    banner->setObjectName("banner");
    banner->setStyleSheet(R"(
        QFrame#banner {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #00C49A, stop:0.5 #00B386, stop:1 #009A73);
            border-radius: 14px;
        }
    )");
    banner->setFixedHeight(140);
    auto* bannerLayout = new QVBoxLayout(banner);
    bannerLayout->setContentsMargins(32, 24, 32, 24);
    bannerLayout->setSpacing(6);

    auto* bannerTitle = new QLabel("发现好电影");
    bannerTitle->setStyleSheet("font-size: 26px; font-weight: bold; color: white; letter-spacing: 2px;");

    auto* bannerSub = new QLabel("接入 WMDB · 豆瓣 · IMDb · 烂番茄 全球影视数据");
    bannerSub->setStyleSheet("font-size: 13px; color: rgba(255,255,255,0.8);");

    auto* bannerHint = new QLabel("在上方搜索框输入关键词开始探索");
    bannerHint->setStyleSheet("font-size: 12px; color: rgba(255,255,255,0.55);");

    bannerLayout->addWidget(bannerTitle);
    bannerLayout->addWidget(bannerSub);
    bannerLayout->addWidget(bannerHint);
    mainLayout->addWidget(banner);

    auto* hotTitle = new QLabel("热门搜索");
    hotTitle->setStyleSheet(R"(
        font-size: 16px;
        font-weight: bold;
        color: #2D2D2D;
        border-left: 3px solid #00B386;
        padding-left: 10px;
    )");
    mainLayout->addWidget(hotTitle);

    m_hotSearchWrap = new QWidget();
    m_hotSearchWrap->setStyleSheet("background: white; border-radius: 12px;");
    m_hotSearchGrid = new QGridLayout(m_hotSearchWrap);
    m_hotSearchGrid->setContentsMargins(HOT_MARGIN, HOT_MARGIN, HOT_MARGIN, HOT_MARGIN);
    m_hotSearchGrid->setSpacing(HOT_SPACING);
    m_hotSearchGrid->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    for (const QString& keyword : HOT_SEARCHES) {
        auto* btn = new QPushButton(keyword);
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        btn->setStyleSheet(R"(
            QPushButton {
                background: #F0FBF7;
                color: #00A370;
                border: 1px solid #B8E8D8;
                border-radius: 14px;
                padding: 6px 14px;
                font-size: 13px;
            }
            QPushButton:hover {
                background: #00B386;
                color: white;
                border-color: #00B386;
            }
            QPushButton:pressed {
                background: #008A63;
                color: white;
                border-color: #008A63;
            }
        )");
        connect(btn, &QPushButton::clicked, this, [this, keyword]() {
            emit movieSearchRequested(keyword);
        });
        m_hotSearchBtns.append(btn);
    }

    rearrangeHotSearch();
    mainLayout->addWidget(m_hotSearchWrap);

    auto* myListTitle = new QLabel("我看过的");
    myListTitle->setStyleSheet(R"(
        font-size: 16px;
        font-weight: bold;
        color: #2D2D2D;
        border-left: 3px solid #FF6000;
        padding-left: 10px;
    )");
    mainLayout->addWidget(myListTitle);

    m_myListWrap = new QWidget();
    m_myListWrap->setStyleSheet("background: white; border-radius: 12px;");
    m_myListGrid = new QGridLayout(m_myListWrap);
    m_myListGrid->setContentsMargins(MY_MARGIN, MY_MARGIN, MY_MARGIN, MY_MARGIN);
    m_myListGrid->setSpacing(MY_CARD_SPACING);
    m_myListGrid->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    m_emptyLabel = new QLabel("还没有观影记录\n去搜索你喜欢的电影吧");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("font-size: 13px; color: #BBB; padding: 40px;");
    m_myListGrid->addWidget(m_emptyLabel, 0, 0, 1, 1, Qt::AlignCenter);

    mainLayout->addWidget(m_myListWrap);

    mainLayout->addStretch();

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);
}

int HomeWidget::calculateHotSearchColumns() const
{
    int availableWidth = m_hotSearchWrap->width() - 2 * HOT_MARGIN;
    if (availableWidth <= 0) availableWidth = 800;
    int cols = availableWidth / (100 + HOT_SPACING);
    int remaining = availableWidth - cols * (100 + HOT_SPACING) + HOT_SPACING;
    if (remaining >= 100) cols++;
    return qMax(1, qMin(cols, 7));
}

int HomeWidget::calculateMyListColumns() const
{
    int availableWidth = m_myListWrap->width() - 2 * MY_MARGIN;
    if (availableWidth <= 0) availableWidth = 800;
    int cols = availableWidth / (MY_CARD_WIDTH + MY_CARD_SPACING);
    int remaining = availableWidth - cols * (MY_CARD_WIDTH + MY_CARD_SPACING) + MY_CARD_SPACING;
    if (remaining >= MY_CARD_WIDTH) cols++;
    return qMax(1, cols);
}

void HomeWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_resizeTimer->start();
}

void HomeWidget::rearrangeHotSearch()
{
    for (auto* btn : m_hotSearchBtns) {
        m_hotSearchGrid->removeWidget(btn);
    }

    int cols = calculateHotSearchColumns();
    if (cols == m_hotSearchCols) {
        for (auto* btn : m_hotSearchBtns) {
            m_hotSearchGrid->addWidget(btn);
        }
        return;
    }
    m_hotSearchCols = cols;

    for (int i = 0; i < m_hotSearchBtns.size(); ++i) {
        int row = i / cols;
        int col = i % cols;
        m_hotSearchGrid->addWidget(m_hotSearchBtns[i], row, col);
    }
}

void HomeWidget::rearrangeMyList()
{
    if (m_watchedData.isEmpty()) return;

    for (auto* card : m_myCards) {
        m_myListGrid->removeWidget(card);
    }

    int cols = calculateMyListColumns();
    if (cols == m_myListCols) {
        for (int i = 0; i < m_myCards.size(); ++i) {
            int row = i / cols;
            int col = i % cols;
            m_myListGrid->addWidget(m_myCards[i], row, col);
        }
        return;
    }
    m_myListCols = cols;

    for (int i = 0; i < m_myCards.size(); ++i) {
        int row = i / cols;
        int col = i % cols;
        m_myListGrid->addWidget(m_myCards[i], row, col);
    }
}

void HomeWidget::refresh()
{
    for (auto* card : m_myCards) {
        m_myListGrid->removeWidget(card);
        delete card;
    }
    m_myCards.clear();
    m_myListCols = 0;

    m_watchedData = m_db->getWatchedList();

    if (m_watchedData.isEmpty()) {
        m_emptyLabel->setVisible(true);
        m_myListGrid->addWidget(m_emptyLabel, 0, 0, 1, 1, Qt::AlignCenter);
        return;
    }

    m_emptyLabel->setVisible(false);
    m_myListGrid->removeWidget(m_emptyLabel);

    int cols = calculateMyListColumns();
    m_myListCols = cols;
    int maxItems = cols * 2;

    for (int i = 0; i < qMin(m_watchedData.size(), maxItems); ++i) {
        const UserReview& r = m_watchedData[i];
        auto* card = new QFrame(m_myListWrap);
        card->setFixedSize(MY_CARD_WIDTH, MY_CARD_HEIGHT);
        card->setStyleSheet(R"(
            QFrame {
                background: white;
                border-radius: 10px;
                border: 1px solid #E8E8E8;
            }
            QFrame:hover {
                border-color: #00B386;
                background: #F0FBF7;
            }
        )");
        card->setCursor(Qt::PointingHandCursor);

        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(8, 10, 8, 10);
        cardLayout->setSpacing(6);

        auto* nameLabel = new QLabel(r.movieName.left(8) + (r.movieName.length() > 8 ? "..." : ""));
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setWordWrap(true);
        nameLabel->setStyleSheet("font-size: 12px; color: #333; font-weight: bold;");

        auto* ratingLabel = new QLabel(r.rating > 0 ? QString("⭐ %1").arg(r.rating, 0, 'f', 1) : "✓ 看过");
        ratingLabel->setAlignment(Qt::AlignCenter);
        ratingLabel->setStyleSheet("font-size: 12px; color: #FF6000; font-weight: bold;");

        cardLayout->addStretch();
        cardLayout->addWidget(nameLabel);
        cardLayout->addWidget(ratingLabel);

        m_myCards.append(card);

        int row = i / cols;
        int col = i % cols;
        m_myListGrid->addWidget(card, row, col);
    }
}
