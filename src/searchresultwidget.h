#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QTimer>
#include "moviemodel.h"
#include "moviecard.h"

class SearchResultWidget : public QWidget {
    Q_OBJECT
public:
    explicit SearchResultWidget(QWidget* parent = nullptr);

    void setResults(const SearchResult& result);
    void appendResults(const SearchResult& result);
    void showLoading(bool show);
    void showEmpty();
    void showError(const QString& error);

signals:
    void movieClicked(const Movie& movie);
    void loadMoreRequested(int skip);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void clearCards();
    void rearrangeCards();
    int calculateColumns() const;

    QScrollArea* m_scrollArea;
    QWidget* m_gridContainer;
    QGridLayout* m_gridLayout;
    QLabel* m_statusLabel;
    QPushButton* m_loadMoreBtn;
    QLabel* m_totalLabel;
    QTimer* m_resizeTimer;

    int m_currentSkip = 0;
    QList<MovieCard*> m_cards;
    QList<Movie> m_allMovies;
    int m_currentCols = 0;
    static constexpr int CARD_WIDTH = 170;
    static constexpr int GRID_SPACING = 18;
    static constexpr int MARGIN = 24;
};
