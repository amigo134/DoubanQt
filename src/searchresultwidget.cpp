#include "searchresultwidget.h"
#include <QScrollBar>
#include <QFrame>

SearchResultWidget::SearchResultWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* infoBar = new QWidget();
    infoBar->setStyleSheet("background: white; border-bottom: 1px solid #ECECEC;");
    infoBar->setFixedHeight(48);
    auto* infoLayout = new QHBoxLayout(infoBar);
    infoLayout->setContentsMargins(24, 0, 24, 0);

    m_totalLabel = new QLabel();
    m_totalLabel->setStyleSheet("font-size: 13px; color: #888;");
    infoLayout->addWidget(m_totalLabel);
    infoLayout->addStretch();
    mainLayout->addWidget(infoBar);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->verticalScrollBar()->setStyleSheet(R"(
        QScrollBar:vertical { width: 8px; background: transparent; }
        QScrollBar::handle:vertical { background: rgba(0,0,0,0.15); border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: rgba(0,0,0,0.25); }
    )");

    auto* scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: #F5F6F8;");
    m_scrollArea->setWidget(scrollContent);

    auto* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    scrollLayout->setSpacing(GRID_SPACING);

    m_gridContainer = new QWidget();
    m_gridContainer->setStyleSheet("background: transparent;");
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setSpacing(GRID_SPACING);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->addWidget(m_gridContainer);

    m_statusLabel = new QLabel();
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 14px; color: #999; padding: 48px;");
    m_statusLabel->setVisible(false);
    scrollLayout->addWidget(m_statusLabel);

    m_loadMoreBtn = new QPushButton("加载更多");
    m_loadMoreBtn->setStyleSheet(R"(
        QPushButton {
            background: white;
            border: 1px solid #DDD;
            border-radius: 22px;
            padding: 11px 44px;
            font-size: 14px;
            color: #555;
        }
        QPushButton:hover {
            background: #F0FBF7;
            border-color: #00B386;
            color: #00B386;
        }
        QPushButton:pressed {
            background: #E8FFF5;
        }
    )");
    m_loadMoreBtn->setVisible(false);
    connect(m_loadMoreBtn, &QPushButton::clicked, this, [this]() {
        emit loadMoreRequested(m_currentSkip);
    });

    auto* loadMoreWrap = new QWidget();
    auto* loadMoreLayout = new QHBoxLayout(loadMoreWrap);
    loadMoreLayout->addStretch();
    loadMoreLayout->addWidget(m_loadMoreBtn);
    loadMoreLayout->addStretch();
    scrollLayout->addWidget(loadMoreWrap);

    scrollLayout->addStretch();
    mainLayout->addWidget(m_scrollArea);

    m_resizeTimer = new QTimer(this);
    m_resizeTimer->setSingleShot(true);
    m_resizeTimer->setInterval(200);
    connect(m_resizeTimer, &QTimer::timeout, this, &SearchResultWidget::rearrangeCards);
}

int SearchResultWidget::calculateColumns() const
{
    int availableWidth = m_scrollArea->viewport()->width() - 2 * MARGIN;
    if (availableWidth <= 0) availableWidth = 800;
    int cols = availableWidth / (CARD_WIDTH + GRID_SPACING);
    int remaining = availableWidth - cols * (CARD_WIDTH + GRID_SPACING) + GRID_SPACING;
    if (remaining >= CARD_WIDTH) cols++;
    return qMax(1, cols);
}

void SearchResultWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_resizeTimer->start();
}

void SearchResultWidget::clearCards()
{
    for (auto* card : m_cards) {
        m_gridLayout->removeWidget(card);
        delete card;
    }
    m_cards.clear();
    m_allMovies.clear();
    m_currentCols = 0;
}

void SearchResultWidget::rearrangeCards()
{
    if (m_cards.isEmpty()) return;

    int cols = calculateColumns();
    if (cols == m_currentCols) return;
    m_currentCols = cols;

    for (auto* card : m_cards) {
        m_gridLayout->removeWidget(card);
    }

    for (int c = 0; c < 20; ++c) {
        m_gridLayout->setColumnStretch(c, 0);
    }

    for (int i = 0; i < m_cards.size(); ++i) {
        int row = i / cols;
        int col = i % cols;
        m_gridLayout->addWidget(m_cards[i], row, col);
    }

    for (int c = 0; c < cols; ++c) {
        m_gridLayout->setColumnStretch(c, 1);
    }
}

void SearchResultWidget::setResults(const SearchResult& result)
{
    clearCards();
    m_allMovies = result.movies;
    m_currentSkip = result.skip + result.count;
    m_statusLabel->setVisible(false);

    if (result.movies.isEmpty()) {
        showEmpty();
        return;
    }

    m_totalLabel->setText(QString("共找到 %1 部作品").arg(result.total));

    int cols = calculateColumns();
    m_currentCols = cols;

    for (int i = 0; i < result.movies.size(); ++i) {
        auto* card = new MovieCard(result.movies[i], m_gridContainer);
        connect(card, &MovieCard::clicked, this, &SearchResultWidget::movieClicked);
        m_cards.append(card);

        int row = i / cols;
        int col = i % cols;
        m_gridLayout->addWidget(card, row, col);
    }

    for (int c = 0; c < cols; ++c) {
        m_gridLayout->setColumnStretch(c, 1);
    }

    m_loadMoreBtn->setVisible(result.hasMore);
}

void SearchResultWidget::appendResults(const SearchResult& result)
{
    m_currentSkip = result.skip + result.count;
    m_allMovies.append(result.movies);

    int cols = calculateColumns();
    m_currentCols = cols;

    int startIndex = m_cards.size();
    for (int i = 0; i < result.movies.size(); ++i) {
        auto* card = new MovieCard(result.movies[i], m_gridContainer);
        connect(card, &MovieCard::clicked, this, &SearchResultWidget::movieClicked);
        m_cards.append(card);

        int pos = startIndex + i;
        int row = pos / cols;
        int col = pos % cols;
        m_gridLayout->addWidget(card, row, col);
    }

    for (int c = 0; c < cols; ++c) {
        m_gridLayout->setColumnStretch(c, 1);
    }

    m_loadMoreBtn->setVisible(result.hasMore);
}

void SearchResultWidget::showLoading(bool show)
{
    if (show) {
        m_statusLabel->setText("正在搜索...");
        m_statusLabel->setVisible(true);
        m_loadMoreBtn->setVisible(false);
        clearCards();
    } else {
        m_statusLabel->setVisible(false);
    }
}

void SearchResultWidget::showEmpty()
{
    m_statusLabel->setText("没有找到相关影视作品\n请尝试其他关键词");
    m_statusLabel->setVisible(true);
    m_totalLabel->setText("");
    m_loadMoreBtn->setVisible(false);
}

void SearchResultWidget::showError(const QString& error)
{
    m_statusLabel->setText("❌ 网络请求失败：\n" + error + "\n\n请检查网络连接后重试");
    m_statusLabel->setStyleSheet("font-size: 14px; color: #E74C3C; padding: 48px;");
    m_statusLabel->setVisible(true);
    m_totalLabel->setText("");
    m_loadMoreBtn->setVisible(false);
}
