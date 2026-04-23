#include "searchresultwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
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

    m_gridContainer = new QWidget();
    m_gridContainer->setStyleSheet("background: #F5F6F8;");
    m_scrollArea->setWidget(m_gridContainer);

    m_outerLayout = new QVBoxLayout(m_gridContainer);
    m_outerLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    m_outerLayout->setSpacing(GRID_SPACING);
    m_outerLayout->addStretch();

    m_statusLabel = new QLabel();
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 14px; color: #999; padding: 48px;");

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

    mainLayout->addWidget(m_scrollArea);
}

int SearchResultWidget::calculateColumns() const
{
    int availableWidth = m_scrollArea->viewport()->width() - 2 * MARGIN;
    if (availableWidth <= 0) availableWidth = 800;
    int cols = availableWidth / (CARD_WIDTH + GRID_SPACING);
    return qMax(1, cols);
}

void SearchResultWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    rearrangeCards();
}

void SearchResultWidget::clearCards()
{
    while (QLayoutItem* item = m_outerLayout->takeAt(0)) {
        QWidget* w = item->widget();
        if (w) w->deleteLater();
        delete item;
    }
    m_row = 0;
}

void SearchResultWidget::addMovieCard(const Movie& movie)
{
    int cols = calculateColumns();
    QHBoxLayout* rowLayout = nullptr;

    if (m_row == 0 || m_outerLayout->count() == 0) {
        rowLayout = new QHBoxLayout();
        rowLayout->setSpacing(GRID_SPACING);
        m_outerLayout->insertLayout(m_outerLayout->count() - 1, rowLayout);
        m_row++;
    } else {
        QLayoutItem* lastItem = m_outerLayout->itemAt(m_outerLayout->count() - 2);
        if (lastItem && lastItem->layout()) {
            rowLayout = qobject_cast<QHBoxLayout*>(lastItem->layout());
        }
        if (!rowLayout) {
            rowLayout = new QHBoxLayout();
            rowLayout->setSpacing(GRID_SPACING);
            m_outerLayout->insertLayout(m_outerLayout->count() - 1, rowLayout);
            m_row++;
        }
    }

    auto* card = new MovieCard(movie, m_gridContainer);
    connect(card, &MovieCard::clicked, this, &SearchResultWidget::movieClicked);
    rowLayout->addWidget(card);

    if (rowLayout->count() >= cols) {
        rowLayout->addStretch();
        m_row++;
    }
}

void SearchResultWidget::rearrangeCards()
{
    if (m_allMovies.isEmpty()) return;

    while (QLayoutItem* item = m_outerLayout->takeAt(0)) {
        QWidget* w = item->widget();
        if (w) w->deleteLater();
        delete item;
    }
    m_row = 0;

    int cols = calculateColumns();
    QHBoxLayout* rowLayout = nullptr;

    for (int i = 0; i < m_allMovies.size(); ++i) {
        if (i % cols == 0) {
            rowLayout = new QHBoxLayout();
            rowLayout->setSpacing(GRID_SPACING);
            m_outerLayout->insertLayout(m_outerLayout->count(), rowLayout);
        }

        auto* card = new MovieCard(m_allMovies[i], m_gridContainer);
        connect(card, &MovieCard::clicked, this, &SearchResultWidget::movieClicked);
        rowLayout->addWidget(card);

        if ((i % cols == cols - 1) || (i == m_allMovies.size() - 1)) {
            rowLayout->addStretch();
        }
    }

    m_outerLayout->addStretch();

    if (m_loadMoreBtn->isVisible()) {
        auto* loadMoreLayout = new QHBoxLayout();
        loadMoreLayout->addStretch();
        loadMoreLayout->addWidget(m_loadMoreBtn);
        loadMoreLayout->addStretch();
        m_outerLayout->addLayout(loadMoreLayout);
    }

    if (m_statusLabel->isVisible()) {
        m_outerLayout->insertWidget(0, m_statusLabel);
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

    rearrangeCards();

    m_loadMoreBtn->setVisible(result.hasMore);
    if (result.hasMore) {
        auto* loadMoreLayout = new QHBoxLayout();
        loadMoreLayout->addStretch();
        loadMoreLayout->addWidget(m_loadMoreBtn);
        loadMoreLayout->addStretch();
        m_outerLayout->addLayout(loadMoreLayout);
    }
}

void SearchResultWidget::appendResults(const SearchResult& result)
{
    m_lastResult = result;
    m_currentSkip = result.skip + result.count;
    m_allMovies.append(result.movies);

    rearrangeCards();

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
        m_outerLayout->insertWidget(0, m_statusLabel);
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
    m_outerLayout->insertWidget(0, m_statusLabel);
}

void SearchResultWidget::showError(const QString& error)
{
    m_statusLabel->setText("❌ 网络请求失败：\n" + error + "\n\n请检查网络连接后重试");
    m_statusLabel->setStyleSheet("font-size: 14px; color: #E74C3C; padding: 48px;");
    m_statusLabel->setVisible(true);
    m_totalLabel->setText("");
    m_loadMoreBtn->setVisible(false);
    m_outerLayout->insertWidget(0, m_statusLabel);
}
