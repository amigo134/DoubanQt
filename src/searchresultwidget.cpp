#include "searchresultwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QScrollBar>

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
    m_gridContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setSpacing(GRID_SPACING);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->addWidget(m_gridContainer, 1);

    m_statusLabel = new QLabel();
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 14px; color: #999; padding: 48px;");
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

    auto* loadMoreContainer = new QWidget();
    auto* loadMoreLayout = new QHBoxLayout(loadMoreContainer);
    loadMoreLayout->addStretch();
    loadMoreLayout->addWidget(m_loadMoreBtn);
    loadMoreLayout->addStretch();
    scrollLayout->addWidget(loadMoreContainer);
    scrollLayout->addStretch();

    mainLayout->addWidget(m_scrollArea);
}

int SearchResultWidget::calculateColumns() const
{
    int availableWidth = m_gridContainer->width();
    if (availableWidth <= 0) availableWidth = width() - 2 * MARGIN;
    int cols = (availableWidth + GRID_SPACING) / (CARD_WIDTH + GRID_SPACING);
    return qMax(1, cols);
}

void SearchResultWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    rearrangeCards();
}

void SearchResultWidget::clearCards()
{
    while (QLayoutItem* item = m_gridLayout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_col = 0;
    m_row = 0;
}

void SearchResultWidget::addMovieCard(const Movie& movie)
{
    auto* card = new MovieCard(movie, m_gridContainer);
    connect(card, &MovieCard::clicked, this, &SearchResultWidget::movieClicked);
    int cols = calculateColumns();
    m_gridLayout->addWidget(card, m_row, m_col, Qt::AlignTop);
    m_col++;
    if (m_col >= cols) {
        m_col = 0;
        m_row++;
    }
}

void SearchResultWidget::rearrangeCards()
{
    if (m_allMovies.isEmpty()) return;

    while (QLayoutItem* item = m_gridLayout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_col = 0;
    m_row = 0;

    int cols = calculateColumns();
    for (const Movie& movie : m_allMovies) {
        auto* card = new MovieCard(movie, m_gridContainer);
        connect(card, &MovieCard::clicked, this, &SearchResultWidget::movieClicked);
        m_gridLayout->addWidget(card, m_row, m_col, Qt::AlignTop);
        m_col++;
        if (m_col >= cols) {
            m_col = 0;
            m_row++;
        }
    }

    for (int c = 0; c < cols; ++c) {
        m_gridLayout->setColumnStretch(c, 1);
    }
}

void SearchResultWidget::setResults(const SearchResult& result)
{
    clearCards();
    m_allMovies = result.movies;
    m_lastResult = result;
    m_currentSkip = result.skip + result.count;
    m_statusLabel->setVisible(false);

    if (result.movies.isEmpty()) {
        showEmpty();
        return;
    }

    m_totalLabel->setText(QString("共找到 %1 部作品").arg(result.total));

    int cols = calculateColumns();
    for (const Movie& movie : result.movies) {
        auto* card = new MovieCard(movie, m_gridContainer);
        connect(card, &MovieCard::clicked, this, &SearchResultWidget::movieClicked);
        m_gridLayout->addWidget(card, m_row, m_col, Qt::AlignTop);
        m_col++;
        if (m_col >= cols) {
            m_col = 0;
            m_row++;
        }
    }

    for (int c = 0; c < cols; ++c) {
        m_gridLayout->setColumnStretch(c, 1);
    }

    m_loadMoreBtn->setVisible(result.hasMore);
}

void SearchResultWidget::appendResults(const SearchResult& result)
{
    m_lastResult = result;
    m_currentSkip = result.skip + result.count;
    m_allMovies.append(result.movies);

    for (const Movie& movie : result.movies) {
        addMovieCard(movie);
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
        m_allMovies.clear();
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
