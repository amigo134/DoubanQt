#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
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

private:
    void clearCards();
    void addMovieCard(const Movie& movie);

    QScrollArea* m_scrollArea;
    QWidget* m_gridContainer;
    QGridLayout* m_gridLayout;
    QLabel* m_statusLabel;
    QPushButton* m_loadMoreBtn;
    QLabel* m_totalLabel;

    int m_currentSkip = 0;
    SearchResult m_lastResult;
    int m_col = 0;
    int m_row = 0;
    static constexpr int COLS = 5;
};
