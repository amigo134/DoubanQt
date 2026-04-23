#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QMovie>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_api(new ApiManager(this))
    , m_db(new DatabaseManager(this))
{
    m_db->initialize();

    setWindowTitle("影视 - 基于 WMDB");
    setMinimumSize(940, 680);
    resize(1100, 760);

    buildUI();

    connect(m_api, &ApiManager::searchResultReady,
            this, &MainWindow::onSearchResultReady);
    connect(m_api, &ApiManager::movieDetailReady,
            this, &MainWindow::onMovieDetailReady);
    connect(m_api, &ApiManager::errorOccurred, this, [this](const QString& err) {
        m_loadingLabel->setVisible(false);
        m_searchResultWidget->showError(err);
    });
    connect(m_api, &ApiManager::networkBusy,
            this, &MainWindow::onNetworkBusy);

    connect(m_searchResultWidget, &SearchResultWidget::movieClicked,
            this, &MainWindow::onMovieClicked);
    connect(m_searchResultWidget, &SearchResultWidget::loadMoreRequested,
            this, &MainWindow::onLoadMore);

    connect(m_detailWidget, &MovieDetailWidget::backRequested,
            this, &MainWindow::onBackFromDetail);
    connect(m_detailWidget, &MovieDetailWidget::reviewUpdated,
            this, [this]() { m_homeWidget->refresh(); });

    connect(m_homeWidget, &HomeWidget::movieSearchRequested,
            this, [this](const QString& q) {
                m_searchEdit->setText(q);
                performSearch(q);
            });
}

void MainWindow::buildUI()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* header = new QWidget();
    header->setFixedHeight(68);
    header->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00C49A, stop:0.5 #00B386, stop:1 #009A73);
        }
    )");
    rootLayout->addWidget(header);

    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(28, 0, 28, 0);
    headerLayout->setSpacing(20);

    auto* logoLabel = new QLabel("🎬 影视");
    logoLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: white; letter-spacing: 3px;");
    headerLayout->addWidget(logoLabel);

    m_navHome = new QPushButton("首页");
    m_navSearch = new QPushButton("搜索结果");
    auto navStyle = [](bool active) {
        return QString(R"(
            QPushButton {
                background: %1;
                color: %2;
                border: none;
                border-radius: 18px;
                padding: 7px 20px;
                font-size: 13px;
                font-weight: bold;
            }
            QPushButton:hover {
                background: rgba(255,255,255,0.25);
                color: white;
            }
        )").arg(active ? "rgba(255,255,255,0.22)" : "transparent")
           .arg(active ? "white" : "rgba(255,255,255,0.75)");
    };
    m_navHome->setStyleSheet(navStyle(true));
    m_navSearch->setStyleSheet(navStyle(false));
    m_navSearch->setVisible(false);

    connect(m_navHome, &QPushButton::clicked, this, [this]() { onNavClicked(HOME); });
    connect(m_navSearch, &QPushButton::clicked, this, [this]() { onNavClicked(SEARCH); });

    headerLayout->addWidget(m_navHome);
    headerLayout->addWidget(m_navSearch);

    headerLayout->addStretch();

    auto* searchContainer = new QFrame();
    searchContainer->setStyleSheet(R"(
        QFrame {
            background: rgba(255,255,255,0.95);
            border-radius: 24px;
        }
    )");
    searchContainer->setFixedHeight(42);
    searchContainer->setFixedWidth(400);
    auto* searchLayout = new QHBoxLayout(searchContainer);
    searchLayout->setContentsMargins(16, 0, 4, 0);
    searchLayout->setSpacing(8);

    auto* searchIcon = new QLabel("🔍");
    searchIcon->setStyleSheet("font-size: 15px; background: transparent;");
    searchLayout->addWidget(searchIcon);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索电影、电视剧、演员...");
    m_searchEdit->setStyleSheet(R"(
        QLineEdit {
            border: none;
            background: transparent;
            font-size: 14px;
            color: #333;
        }
        QLineEdit::placeholder {
            color: #AAA;
        }
    )");
    searchLayout->addWidget(m_searchEdit);

    auto* searchBtn = new QPushButton("搜索");
    searchBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #00C49A, stop:1 #009A73);
            color: white;
            border: none;
            border-radius: 20px;
            padding: 7px 20px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #00D4AA, stop:1 #00AA83);
        }
        QPushButton:pressed {
            background: #008A63;
        }
    )");
    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearch);
    searchLayout->addWidget(searchBtn);

    headerLayout->addWidget(searchContainer);

    m_loadingLabel = new QLabel("⏳ 搜索中...");
    m_loadingLabel->setStyleSheet("color: rgba(255,255,255,0.85); font-size: 13px; font-weight: bold;");
    m_loadingLabel->setVisible(false);
    headerLayout->addWidget(m_loadingLabel);

    m_stackedWidget = new QStackedWidget();
    rootLayout->addWidget(m_stackedWidget);

    m_homeWidget = new HomeWidget(m_db, this);
    m_searchResultWidget = new SearchResultWidget(this);
    m_detailWidget = new MovieDetailWidget(m_db, this);

    m_stackedWidget->addWidget(m_homeWidget);
    m_stackedWidget->addWidget(m_searchResultWidget);
    m_stackedWidget->addWidget(m_detailWidget);

    m_stackedWidget->setCurrentIndex(HOME);
}

void MainWindow::onSearch()
{
    QString query = m_searchEdit->text().trimmed();
    if (query.isEmpty()) return;
    performSearch(query);
}

void MainWindow::performSearch(const QString& query, int skip)
{
    m_currentQuery = query;
    if (skip == 0) {
        m_searchResultWidget->showLoading(true);
        m_stackedWidget->setCurrentIndex(SEARCH);
        m_navSearch->setVisible(true);
        m_navSearch->setStyleSheet(R"(
            QPushButton {
                background: rgba(255,255,255,0.22);
                color: white;
                border: none;
                border-radius: 18px;
                padding: 7px 20px;
                font-size: 13px;
                font-weight: bold;
            }
        )");
    }
    m_api->searchMovies(query, QString(), 0, 20, skip);
}

void MainWindow::onSearchResultReady(const SearchResult& result)
{
    if (result.skip == 0) {
        m_searchResultWidget->setResults(result);
    } else {
        m_searchResultWidget->appendResults(result);
    }
}

void MainWindow::onLoadMore(int skip)
{
    m_api->searchMovies(m_currentQuery, QString(), 0, 20, skip);
}

void MainWindow::onMovieClicked(const Movie& movie)
{
    if (movie.doubanId.isEmpty()) {
        m_detailWidget->setMovie(movie);
        m_stackedWidget->setCurrentIndex(DETAIL);
    } else {
        m_api->getMovieById(movie.doubanId);
        m_stackedWidget->setCurrentIndex(DETAIL);
        Movie loadingMovie = movie;
        m_detailWidget->setMovie(loadingMovie);
    }
}

void MainWindow::onMovieDetailReady(const Movie& movie)
{
    m_detailWidget->setMovie(movie);
}

void MainWindow::onBackFromDetail()
{
    m_stackedWidget->setCurrentIndex(
        m_navSearch->isVisible() && !m_currentQuery.isEmpty() ? SEARCH : HOME
    );
}

void MainWindow::onNavClicked(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    auto activeStyle = R"(
        QPushButton {
            background: rgba(255,255,255,0.22);
            color: white;
            border: none;
            border-radius: 18px;
            padding: 7px 20px;
            font-size: 13px;
            font-weight: bold;
        }
    )";
    auto inactiveStyle = R"(
        QPushButton {
            background: transparent;
            color: rgba(255,255,255,0.65);
            border: none;
            border-radius: 18px;
            padding: 7px 20px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: rgba(255,255,255,0.15);
            color: white;
        }
    )";
    m_navHome->setStyleSheet(index == HOME ? activeStyle : inactiveStyle);
    m_navSearch->setStyleSheet(index == SEARCH ? activeStyle : inactiveStyle);

    if (index == HOME) {
        m_homeWidget->refresh();
    }
}

void MainWindow::onNetworkBusy(bool busy)
{
    m_loadingLabel->setVisible(busy);
}
