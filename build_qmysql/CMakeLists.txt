cmake_minimum_required(VERSION 3.16)
project(qsqlmysql LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/5.15.2/mingw81_64")

find_package(Qt5 REQUIRED COMPONENTS Core Sql)

set(MYSQL_INCLUDE "C:/Program Files/MySQL/MySQL Server 8.0/include")
set(MYSQL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(QT_SQL_SOURCES_DIR "C:/Qt/5.15.2/Src/qtbase/src/plugins/sqldrivers/mysql")

add_library(qsqlmysql SHARED
    ${QT_SQL_SOURCES_DIR}/main.cpp
    ${QT_SQL_SOURCES_DIR}/qsql_mysql.cpp
)

target_include_directories(qsqlmysql PRIVATE
    ${QT_SQL_SOURCES_DIR}
    ${MYSQL_INCLUDE}
    "C:/Qt/5.15.2/mingw81_64/include/QtSql/5.15.2"
    "C:/Qt/5.15.2/mingw81_64/include/QtSql/5.15.2/QtSql"
    "C:/Qt/5.15.2/mingw81_64/include/QtCore/5.15.2"
    "C:/Qt/5.15.2/mingw81_64/include/QtCore/5.15.2/QtCore"
)

target_link_libraries(qsqlmysql PRIVATE
    Qt5::Core
    Qt5::Sql
    ${MYSQL_LIB_DIR}/libmysql.a
)

target_compile_definitions(qsqlmysql PRIVATE
    QT_PLUGIN
    QT_NO_CAST_TO_ASCII
    QT_NO_CAST_FROM_ASCII
    Q_EXPORT_SQLDRIVER_MYSQL=
)

set_target_properties(qsqlmysql PROPERTIES
    PREFIX ""
    OUTPUT_NAME "qsqlmysql"
)
