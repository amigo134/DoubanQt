cmake_minimum_required(VERSION 3.16)
project(DoubanChatServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/5.15.2/mingw81_64")

find_package(Qt5 REQUIRED COMPONENTS Core WebSockets Sql Concurrent)

add_executable(ChatServer
    main.cpp
    chatserver.cpp
    serverdb.cpp
    connectionpool.cpp
)

target_link_libraries(ChatServer PRIVATE
    Qt5::Core
    Qt5::WebSockets
    Qt5::Sql
    Qt5::Concurrent
)

target_include_directories(ChatServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
