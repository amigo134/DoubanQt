#include "homewidget.h"
#include <QScrollArea>
#include <QScrollBar>
#include <QFrame>

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
        rebuildHotSearchRows();
        rebuildMyListRows();
    });
}

void HomeWidget::buildUI()
{
    setStyleSheet("background: #F5F6F8;");

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

    auto* banner = new QFrame();
    banner->setObjectName("banner");
    banner->setStyleSheet(R"(
        QFrame#banner {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #00C49A, stop:1 #009A73);
            border-radius: 12px;
        }
    )");
    banner->setFixedHeight(120);
    auto* bl = new QVBoxLayout(banner);
    bl->setContentsMargins(28, 20, 28, 20);
    bl->setSpacing(4);
    auto* t1 = new QLabel("发现好电影");
    t1->setStyleSheet("font-size: 22px; font-weight: bold; color: white;");
    auto* t2 = new QLabel("WMDB · 豆瓣 · IMDb · 烂番茄 全球影视数据");
    t2->setStyleSheet("font-size: 12px; color: rgba(255,255,255,0.75);");
    bl->addWidget(t1);
    bl->addWidget(t2);
    root->addWidget(banner);

    auto* hotTitle = new QLabel("热门搜索");
    hotTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #333; border-left: 3px solid #00B386; padding-left: 8px;");
    root->addWidget(hotTitle);

    m_hotSearchWrap = new QWidget();
    m_hotSearchWrap->setStyleSheet("background: white; border-radius: 10px;");
    m_hotSearchOuterLayout = new QVBoxLayout(m_hotSearchWrap);
    m_hotSearchOuterLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    m_hotSearchOuterLayout->setSpacing(SPACING);

    for (const QString& kw : HOT_SEARCHES) {
        auto* btn = new QPushButton(kw);
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        btn->setStyleSheet(R"(
            QPushButton {
                background: #F0FBF7; color: #00A370;
                border: 1px solid #B8E8D8; border-radius: 14px;
                padding: 5px 14px; font-size: 13px;
            }
            QPushButton:hover { background: #00B386; color: white; border-color: #00B386; }
            QPushButton:pressed { background: #008A63; color: white; }
        )");
        connect(btn, &QPushButton::clicked, this, [this, kw]() {
            emit movieSearchRequested(kw);
        });
        m_hotBtns.append(btn);
    }

    rebuildHotSearchRows();
    root->addWidget(m_hotSearchWrap);

    auto* myListTitle = new QLabel("我看过的");
    myListTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #333; border-left: 3px solid #FF6000; padding-left: 8px;");
    root->addWidget(myListTitle);

    m_myListWrap = new QWidget();
    m_myListWrap->setStyleSheet("background: white; border-radius: 10px;");
    m_myListOuterLayout = new QVBoxLayout(m_myListWrap);
    m_myListOuterLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    m_myListOuterLayout->setSpacing(SPACING);

    m_emptyLabel = new QLabel("还没有观影记录\n去搜索你喜欢的电影吧");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("font-size: 13px; color: #BBB; padding: 36px;");
    m_myListOuterLayout->addWidget(m_emptyLabel);

    root->addWidget(m_myListWrap);
    root->addStretch();

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);
}

int HomeWidget::calcHotSearchCols() const
{
    int w = m_hotSearchWrap->width() - 2 * MARGIN;
    if (w <= 0) w = 800;
    int cols = w / (100 + SPACING);
    int rem = w - cols * (100 + SPACING) + SPACING;
    if (rem >= 100) cols++;
    return qMax(1, qMin(cols, 7));
}

int HomeWidget::calcMyListCols() const
{
    int w = m_myListWrap->width() - 2 * MARGIN;
    if (w <= 0) w = 800;
    int cols = w / (MY_CARD_W + SPACING);
    int rem = w - cols * (MY_CARD_W + SPACING) + SPACING;
    if (rem >= MY_CARD_W) cols++;
    return qMax(1, cols);
}

void HomeWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_resizeTimer->start();
}

void HomeWidget::rebuildHotSearchRows()
{
    while (QLayoutItem* item = m_hotSearchOuterLayout->takeAt(0)) {
        QLayout* sub = item->layout();
        if (sub) {
            while (QLayoutItem* si = sub->takeAt(0)) delete si;
            delete sub;
        }
        delete item;
    }

    int cols = calcHotSearchCols();
    m_hotCols = cols;

    QHBoxLayout* row = nullptr;
    for (int i = 0; i < m_hotBtns.size(); ++i) {
        if (i % cols == 0) {
            row = new QHBoxLayout();
            row->setSpacing(SPACING);
            row->setAlignment(Qt::AlignLeft);
            m_hotSearchOuterLayout->addLayout(row);
        }
        row->addWidget(m_hotBtns[i]);
    }
}

void HomeWidget::rebuildMyListRows()
{
    if (m_myCards.isEmpty()) return;

    while (QLayoutItem* item = m_myListOuterLayout->takeAt(0)) {
        QLayout* sub = item->layout();
        if (sub) {
            while (QLayoutItem* si = sub->takeAt(0)) delete si;
            delete sub;
        }
        delete item;
    }

    int cols = calcMyListCols();
    m_myCols = cols;

    QHBoxLayout* row = nullptr;
    for (int i = 0; i < m_myCards.size(); ++i) {
        if (i % cols == 0) {
            row = new QHBoxLayout();
            row->setSpacing(SPACING);
            row->setAlignment(Qt::AlignLeft);
            m_myListOuterLayout->addLayout(row);
        }
        row->addWidget(m_myCards[i]);
    }
}

void HomeWidget::refresh()
{
    for (auto* card : m_myCards) {
        m_myListOuterLayout->removeWidget(card);
        delete card;
    }
    m_myCards.clear();
    m_myCols = 0;

    m_watchedData = m_db->getWatchedList();

    if (m_watchedData.isEmpty()) {
        m_emptyLabel->setVisible(true);
        while (QLayoutItem* item = m_myListOuterLayout->takeAt(0)) {
            QLayout* sub = item->layout();
            if (sub) { while (QLayoutItem* si = sub->takeAt(0)) delete si; delete sub; }
            delete item;
        }
        m_myListOuterLayout->addWidget(m_emptyLabel);
        return;
    }

    m_emptyLabel->setVisible(false);
    m_myListOuterLayout->removeWidget(m_emptyLabel);

    while (QLayoutItem* item = m_myListOuterLayout->takeAt(0)) {
        QLayout* sub = item->layout();
        if (sub) { while (QLayoutItem* si = sub->takeAt(0)) delete si; delete sub; }
        delete item;
    }

    int cols = calcMyListCols();
    m_myCols = cols;
    int maxItems = cols * 2;

    QHBoxLayout* row = nullptr;
    for (int i = 0; i < qMin(m_watchedData.size(), maxItems); ++i) {
        const UserReview& r = m_watchedData[i];
        auto* card = new QFrame(m_myListWrap);
        card->setFixedSize(MY_CARD_W, MY_CARD_H);
        card->setStyleSheet(R"(
            QFrame {
                background: white; border-radius: 8px;
                border: 1px solid #E8E8E8;
            }
            QFrame:hover { border-color: #00B386; background: #F0FBF7; }
        )");
        card->setCursor(Qt::PointingHandCursor);

        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(6, 8, 6, 8);
        cl->setSpacing(4);

        auto* nm = new QLabel(r.movieName.left(8) + (r.movieName.length() > 8 ? "..." : ""));
        nm->setAlignment(Qt::AlignCenter);
        nm->setWordWrap(true);
        nm->setStyleSheet("font-size: 12px; color: #333; font-weight: bold;");

        auto* rt = new QLabel(r.rating > 0 ? QString("⭐ %1").arg(r.rating, 0, 'f', 1) : "✓ 看过");
        rt->setAlignment(Qt::AlignCenter);
        rt->setStyleSheet("font-size: 11px; color: #FF6000; font-weight: bold;");

        cl->addStretch();
        cl->addWidget(nm);
        cl->addWidget(rt);

        m_myCards.append(card);

        if (i % cols == 0) {
            row = new QHBoxLayout();
            row->setSpacing(SPACING);
            row->setAlignment(Qt::AlignLeft);
            m_myListOuterLayout->addLayout(row);
        }
        row->addWidget(card);
    }
}
