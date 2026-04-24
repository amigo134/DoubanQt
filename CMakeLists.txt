cmake_minimum_required(VERSION 3.16)
project(DoubanQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt5 路径（mingw 版本）
set(CMAKE_PREFIX_PATH "C:/Qt/5.15.2/mingw81_64")

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Sql)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/apimanager.cpp
    src/databasemanager.cpp
    src/moviedetailwidget.cpp
    src/moviecard.cpp
    src/ratingwidget.cpp
    src/reviewdialog.cpp
    src/searchresultwidget.cpp
    src/homewidget.cpp
    src/imagecache.cpp
    src/profilewidget.cpp
)

set(HEADERS
    src/mainwindow.h
    src/apimanager.h
    src/databasemanager.h
    src/moviedetailwidget.h
    src/moviecard.h
    src/ratingwidget.h
    src/reviewdialog.h
    src/searchresultwidget.h
    src/homewidget.h
    src/imagecache.h
    src/moviemodel.h
    src/profilewidget.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(DoubanQt
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(DoubanQt PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::Sql
)

target_include_directories(DoubanQt PRIVATE src)
