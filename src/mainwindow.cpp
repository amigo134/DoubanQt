#include "mainwindow.h"
#include "logindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QMovie>
#include <QKeyEvent>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_api(new ApiManager(this))
    , m_db(new DatabaseManager(this))
{
    m_db->initialize();

    if (!m_db->isReady()) {
        QMessageBox::critical(nullptr, "数据库错误",
            "无法打开或创建数据库，请检查程序是否有写入权限。\n\n"
            "数据库路径通常位于：\n"
            "C:/Users/<用户名>/AppData/Local/DoubanQt/DoubanQt/");
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        return;
    }

    if (!showLogin()) {
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        return;
    }

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
            this, [this]() { m_homeWidget->refresh(); m_profileWidget->refresh(); });

    connect(m_homeWidget, &HomeWidget::movieSearchRequested,
            this, [this](const QString& q) {
                m_searchEdit->setText(q);
                performSearch(q);
            });
    connect(m_homeWidget, &HomeWidget::top250Requested,
            this, [this]() { m_api->getTop250(); });
    connect(m_api, &ApiManager::top250Ready,
            m_homeWidget, &HomeWidget::setTop250Data);
    connect(m_homeWidget, &HomeWidget::movieClicked,
            this, &MainWindow::onMovieClicked);

    connect(m_profileWidget, &ProfileWidget::movieClicked,
            this, &MainWindow::onMovieClicked);
    connect(m_profileWidget, &ProfileWidget::logoutRequested,
            this, &MainWindow::onLogout);
}

void MainWindow::buildUI()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* header = new QWidget();
    header->setFixedHeight(60);
    header->setStyleSheet(R"(
        QWidget {
            background: white;
            border-bottom: 1px solid #E8E8EC;
        }
    )");
    rootLayout->addWidget(header);

    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(32, 0, 32, 0);
    headerLayout->setSpacing(12);

    auto* logoLabel = new QLabel("DOUBAN");
    logoLabel->setStyleSheet(R"(
        font-size: 20px;
        font-weight: 900;
        color: #00B51D;
        letter-spacing: 4px;
        font-family: "Arial Black", "Microsoft YaHei UI";
    )");
    headerLayout->addWidget(logoLabel);

    auto* dotLabel = new QLabel("MOVIE");
    dotLabel->setStyleSheet(R"(
        font-size: 12px;
        font-weight: bold;
        color: #999;
        letter-spacing: 2px;
        margin-top: 4px;
    )");
    headerLayout->addWidget(dotLabel);

    m_navHome = new QPushButton("首页");
    m_navSearch = new QPushButton("搜索结果");
    m_navProfile = new QPushButton("我的");
    auto navStyle = [](bool active) {
        return QString(R"(
            QPushButton {
                background: %1;
                color: %2;
                border: none;
                border-radius: 6px;
                padding: 6px 16px;
                font-size: 13px;
                font-weight: bold;
            }
            QPushButton:hover {
                background: #F0F0F2;
                color: #333;
            }
        )").arg(active ? "#E8F5E9" : "transparent")
           .arg(active ? "#00B51D" : "#888");
    };
    m_navHome->setStyleSheet(navStyle(true));
    m_navSearch->setStyleSheet(navStyle(false));
    m_navSearch->setVisible(false);
    m_navProfile->setStyleSheet(navStyle(false));

    connect(m_navHome, &QPushButton::clicked, this, [this]() { onNavClicked(HOME); });
    connect(m_navSearch, &QPushButton::clicked, this, [this]() { onNavClicked(SEARCH); });
    connect(m_navProfile, &QPushButton::clicked, this, [this]() { onNavClicked(PROFILE); });

    headerLayout->addWidget(m_navHome);
    headerLayout->addWidget(m_navSearch);
    headerLayout->addWidget(m_navProfile);

    headerLayout->addStretch();

    auto* searchContainer = new QFrame();
    searchContainer->setStyleSheet(R"(
        QFrame {
            background: #F5F5F7;
            border: 1px solid #E0E0E4;
            border-radius: 8px;
        }
        QFrame:hover {
            border-color: #00B51D;
            background: white;
        }
    )");
    searchContainer->setFixedHeight(38);
    searchContainer->setMinimumWidth(280);
    searchContainer->setMaximumWidth(480);
    m_searchContainer = searchContainer;
    auto* searchLayout = new QHBoxLayout(searchContainer);
    searchLayout->setContentsMargins(12, 0, 4, 0);
    searchLayout->setSpacing(6);

    auto* searchIcon = new QLabel("🔍");
    searchIcon->setStyleSheet("font-size: 14px; background: transparent;");
    searchLayout->addWidget(searchIcon);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索电影、电视剧、演员...");
    m_searchEdit->setStyleSheet(R"(
        QLineEdit {
            border: none;
            background: transparent;
            font-size: 13px;
            color: #222;
        }
        QLineEdit::placeholder {
            color: #BBB;
        }
    )");
    searchLayout->addWidget(m_searchEdit);

    auto* searchBtn = new QPushButton("搜索");
    searchBtn->setStyleSheet(R"(
        QPushButton {
            background: #00B51D;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 6px 18px;
            font-size: 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #009A18;
        }
        QPushButton:pressed {
            background: #008012;
        }
    )");
    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearch);
    searchLayout->addWidget(searchBtn);

    headerLayout->addWidget(searchContainer);

    m_loadingLabel = new QLabel("搜索中...");
    m_loadingLabel->setStyleSheet("color: #999; font-size: 12px;");
    m_loadingLabel->setVisible(false);
    headerLayout->addWidget(m_loadingLabel);

    m_stackedWidget = new QStackedWidget();
    rootLayout->addWidget(m_stackedWidget);

    m_homeWidget = new HomeWidget(m_db, this);
    m_searchResultWidget = new SearchResultWidget(this);
    m_detailWidget = new MovieDetailWidget(m_db, this);
    m_profileWidget = new ProfileWidget(m_db, this);

    m_stackedWidget->addWidget(m_homeWidget);
    m_stackedWidget->addWidget(m_searchResultWidget);
    m_stackedWidget->addWidget(m_detailWidget);
    m_stackedWidget->addWidget(m_profileWidget);

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
        onNavClicked(SEARCH);
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
    m_prevPage = m_stackedWidget->currentIndex();
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
    m_db->updatePosterUrl(movie.doubanId, movie.getPoster());
}

void MainWindow::onBackFromDetail()
{
    int page = m_prevPage;
    if (page == DETAIL) page = HOME;
    m_stackedWidget->setCurrentIndex(page);
    onNavClicked(page);
}

void MainWindow::onNavClicked(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    auto activeStyle = R"(
        QPushButton {
            background: #E8F5E9;
            color: #00B51D;
            border: none;
            border-radius: 6px;
            padding: 6px 16px;
            font-size: 13px;
            font-weight: bold;
        }
    )";
    auto inactiveStyle = R"(
        QPushButton {
            background: transparent;
            color: #888;
            border: none;
            border-radius: 6px;
            padding: 6px 16px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #F0F0F2;
            color: #333;
        }
    )";
    m_navHome->setStyleSheet(index == HOME ? activeStyle : inactiveStyle);
    m_navSearch->setStyleSheet(index == SEARCH ? activeStyle : inactiveStyle);
    m_navProfile->setStyleSheet(index == PROFILE ? activeStyle : inactiveStyle);

    if (index == HOME) {
        m_homeWidget->refresh();
    } else if (index == PROFILE) {
        m_profileWidget->refresh();
    }
}

void MainWindow::onNetworkBusy(bool busy)
{
    m_loadingLabel->setVisible(busy);
}

void MainWindow::onLogout()
{
    if (!showLogin()) {
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        return;
    }
    m_homeWidget->refresh();
    m_profileWidget->refresh();
    m_stackedWidget->setCurrentIndex(HOME);
    onNavClicked(HOME);
}

bool MainWindow::showLogin()
{
    while (true) {
        LoginDialog dlg;
        if (!m_db->hasUsers()) {
            dlg.switchToRegister();
        }
        if (dlg.exec() != QDialog::Accepted) return false;

        QString username = dlg.getUsername();
        QString password = dlg.getPassword();

        if (!m_db->isReady()) {
            QMessageBox::critical(nullptr, "数据库错误",
                "数据库不可用，无法登录。请重启程序重试。");
            return false;
        }

        if (dlg.isRegister()) {
            int id = m_db->registerUser(username, password);
            if (id > 0) {
                m_db->setCurrentUser(id);
                return true;
            }
            QMessageBox::warning(nullptr, "注册失败", "用户名已存在，请换一个");
        } else {
            int id = m_db->loginUser(username, password);
            if (id > 0) {
                m_db->setCurrentUser(id);
                return true;
            }
            QMessageBox::warning(nullptr, "登录失败", "用户名或密码错误");
        }
    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateSearchBoxWidth();
}

void MainWindow::updateSearchBoxWidth()
{
    if (!m_searchContainer) return;
    int windowWidth = width();
    int searchWidth = qBound(280, windowWidth / 3, 480);
    m_searchContainer->setFixedWidth(searchWidth);
}
