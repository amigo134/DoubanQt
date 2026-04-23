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
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(24);

    auto* banner = new QFrame();
    banner->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #00C49A, stop:0.5 #00B386, stop:1 #009A73);
            border-radius: 16px;
        }
    )");
    banner->setFixedHeight(160);
    auto* bannerLayout = new QVBoxLayout(banner);
    bannerLayout->setContentsMargins(36, 28, 36, 28);
    bannerLayout->setSpacing(8);
    auto* bannerTitle = new QLabel("✨ 发现好电影");
    bannerTitle->setStyleSheet("font-size: 28px; font-weight: bold; color: white; letter-spacing: 2px;");
    auto* bannerSub = new QLabel("接入 WMDB · 豆瓣 · IMDb · 烂番茄 全球影视数据");
    bannerSub->setStyleSheet("font-size: 14px; color: rgba(255,255,255,0.8);");
    auto* bannerHint = new QLabel("🔍 在上方搜索框输入关键词开始探索");
    bannerHint->setStyleSheet("font-size: 13px; color: rgba(255,255,255,0.6);");
    bannerLayout->addWidget(bannerTitle);
    bannerLayout->addWidget(bannerSub);
    bannerLayout->addWidget(bannerHint);
    mainLayout->addWidget(banner);

    buildHotSearchSection(mainLayout);

    buildMyListSection(mainLayout);

    mainLayout->addStretch();

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);
}

void HomeWidget::buildHotSearchSection(QVBoxLayout* layout)
{
    auto* sectionTitle = new QLabel("🔥 热门搜索");
    sectionTitle->setStyleSheet(R"(
        font-size: 17px;
        font-weight: bold;
        color: #2D2D2D;
        border-left: 4px solid #00B386;
        padding-left: 12px;
    )");
    layout->addWidget(sectionTitle);

    auto* wrap = new QWidget();
    wrap->setStyleSheet("background: white; border-radius: 12px;");
    auto* wrapMain = new QVBoxLayout(wrap);
    wrapMain->setContentsMargins(18, 18, 18, 18);
    wrapMain->setSpacing(12);

    int perRow = 6;
    QHBoxLayout* rowLayout = nullptr;

    for (int i = 0; i < HOT_SEARCHES.size(); ++i) {
        if (i % perRow == 0) {
            rowLayout = new QHBoxLayout();
            rowLayout->setSpacing(10);
            rowLayout->setAlignment(Qt::AlignLeft);
            wrapMain->addLayout(rowLayout);
        }

        const QString& keyword = HOT_SEARCHES[i];
        auto* btn = new QPushButton(keyword);
        btn->setStyleSheet(R"(
            QPushButton {
                background: #F0FBF7;
                color: #00A370;
                border: 1px solid #B8E8D8;
                border-radius: 16px;
                padding: 7px 16px;
                font-size: 13px;
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #00C49A, stop:1 #009A73);
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
        rowLayout->addWidget(btn);
    }
    if (rowLayout) rowLayout->addStretch();

    layout->addWidget(wrap);
}

void HomeWidget::buildMyListSection(QVBoxLayout* layout)
{
    auto* sectionTitle = new QLabel("📖 我看过的");
    sectionTitle->setStyleSheet(R"(
        font-size: 17px;
        font-weight: bold;
        color: #2D2D2D;
        border-left: 4px solid #FF6000;
        padding-left: 12px;
    )");
    layout->addWidget(sectionTitle);

    m_myListWidget = new QWidget();
    m_myListWidget->setStyleSheet("background: white; border-radius: 12px;");
    m_myListGrid = new QGridLayout(m_myListWidget);
    m_myListGrid->setContentsMargins(18, 18, 18, 18);
    m_myListGrid->setSpacing(14);

    m_emptyLabel = new QLabel("还没有观影记录\n去搜索你喜欢的电影吧 🎬");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("font-size: 14px; color: #BBB; padding: 48px;");
    m_myListGrid->addWidget(m_emptyLabel, 0, 0, 1, 5, Qt::AlignCenter);

    layout->addWidget(m_myListWidget);

    refreshMyList();
}

void HomeWidget::refresh()
{
    refreshMyList();
}

void HomeWidget::refreshMyList()
{
    while (QLayoutItem* item = m_myListGrid->takeAt(0)) {
        QWidget* w = item->widget();
        if (w && w != m_emptyLabel) {
            w->deleteLater();
        }
        delete item;
    }

    QList<UserReview> watched = m_db->getWatchedList();
    if (watched.isEmpty()) {
        m_myListGrid->addWidget(m_emptyLabel, 0, 0, 1, 5, Qt::AlignCenter);
        m_emptyLabel->setVisible(true);
        return;
    }

    m_emptyLabel->setVisible(false);
    int col = 0, row = 0;
    for (const UserReview& r : watched) {
        auto* card = new QFrame(m_myListWidget);
        card->setFixedSize(110, 140);
        card->setStyleSheet(R"(
            QFrame {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #FAFAFA, stop:1 #F0F0F0);
                border-radius: 10px;
                border: 1px solid #E8E8E8;
            }
            QFrame:hover {
                border-color: #00B386;
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #F0FBF7, stop:1 #E8F8F0);
            }
        )");
        card->setCursor(Qt::PointingHandCursor);

        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(8, 8, 8, 8);
        cardLayout->setSpacing(4);

        auto* nameLabel = new QLabel(r.movieName.left(8) + (r.movieName.length() > 8 ? "..." : ""));
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setWordWrap(true);
        nameLabel->setStyleSheet("font-size: 12px; color: #444; font-weight: bold;");

        auto* ratingLabel = new QLabel(r.rating > 0 ? QString("⭐ %1").arg(r.rating, 0, 'f', 1) : "✓ 看过");
        ratingLabel->setAlignment(Qt::AlignCenter);
        ratingLabel->setStyleSheet("font-size: 12px; color: #FF6000; font-weight: bold;");

        cardLayout->addStretch();
        cardLayout->addWidget(nameLabel);
        cardLayout->addWidget(ratingLabel);

        m_myListGrid->addWidget(card, row, col);
        col++;
        if (col >= 6) { col = 0; row++; }

        if (row > 1 && col == 0) break;
    }
}
