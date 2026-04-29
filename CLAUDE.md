# DoubanQt

豆瓣电影浏览 + 聊天客户端/服务端，C++17 + Qt 5.15.2，MinGW 64-bit。

## 项目结构

```
dou/
├── src/                    # 客户端 Qt Widgets 应用
│   ├── main.cpp            # 入口，全局样式，QApplication
│   ├── mainwindow.h/cpp    # 主窗口，QStackedWidget 页面切换
│   ├── apimanager.h/cpp    # 豆瓣 API 调用（搜索、详情、Top250），QNetworkAccessManager
│   ├── databasemanager.h/cpp # 客户端本地 DB（用户、影评、想看/看过），QSqlDatabase
│   ├── chatmanager.h/cpp   # WebSocket 聊天客户端，连接 ChatServer
│   ├── chatmodel.h/cpp     # 聊天消息列表 Model（QAbstractListModel）
│   ├── chatbubble.h/cpp    # 聊天气泡绘制
│   ├── chatmessagewidget.h/cpp    # 聊天界面
│   ├── chatmessagedelegate.h/cpp  # 聊天列表项委托
│   ├── friendswidget.h/cpp # 好友列表
│   ├── homewidget.h/cpp    # 首页（Top250、搜索框）
│   ├── searchresultwidget.h/cpp   # 搜索结果页
│   ├── moviedetailwidget.h/cpp    # 电影详情页
│   ├── moviecard.h/cpp     # 电影卡片组件
│   ├── ratingwidget.h/cpp  # 评分星星组件
│   ├── reviewdialog.h/cpp  # 写影评弹窗
│   ├── profilewidget.h/cpp / profileeditdialog.h/cpp  # 个人主页/编辑
│   ├── logindialog.h/cpp   # 登录/注册弹窗
│   ├── avatarwidget.h/cpp  # 头像组件
│   ├── imagecache.h/cpp    # 图片缓存
│   └── loadingwidget.h     # 加载遮罩
├── server/                 # 聊天服务端，独立可执行文件 ChatServer
│   ├── main.cpp            # QCoreApplication，监听 8765
│   ├── chatserver.h/cpp    # QWebSocketServer，事件驱动，DB 异步化（runDbAsync）
│   ├── serverdb.h/cpp      # MySQL 数据库层（用户、好友、消息）
│   └── connectionpool.h/cpp # MySQL 连接池（per-thread 懒创建，5连接/线程）
├── resources/              # QRC 资源（图标、图片）
├── CMakeLists.txt          # 根构建（客户端 + add_subdirectory(server)）
├── server/CMakeLists.txt   # 服务端构建
└── build.bat               # 构建脚本
```

## 构建

- Qt 路径：`C:/Qt/5.15.2/mingw81_64`
- 客户端构建目录：`build/`，输出 `DoubanQt.exe`
- 服务端构建目录：`build_server/`，输出 `ChatServer.exe`
- 服务端需要 `libmysql.dll` 和 `sqldrivers/` 放在 exe 同目录
- `build.bat` 一键构建客户端 + 服务端

## 关键架构

### 客户端
- **页面管理**：MainWindow 用 QStackedWidget，页面索引常量 0=首页, 1=搜索结果, 2=电影详情, 3=个人, 4=好友, 5=聊天
- **API 通信**：ApiManager 请求 movie.lovestudio.top 的外部 API
- **本地存储**：DatabaseManager 存用户账号、影评、想看/看过列表
- **聊天**：ChatManager 通过 WebSocket 连 ChatServer，管理登录、好友、消息

### 服务端
- WebSocket I/O 主线程，DB 操作通过 QtConcurrent::run 异步化到 QThreadPool
- 每个消息 type 映射到 handler
- 用户上线/离线通知好友；消息在线转发，离线存库推上线
- ConnectionPool 支持 per-thread 懒创建连接（5连接/线程）

### 数据模型
- `Movie` / `SearchResult` / `UserReview` 定义在 `moviemodel.h`
- `ChatMsg` / `FriendInfo` / `ChatItem` 定义在 `chatmodel.h`
- `ServerMsg` 定义在 `serverdb.h`

## 注意事项
- 不要改 build/ 和 build_server/ 下自动生成的文件（moc_*, CMakeFiles）
- dist/ 目录是发布用的
- Git 仓库：https://github.com/amigo134/DoubanQt
- 服务端 DB 连接参数硬编码在 serverdb.cpp（localhost:3306, root/123456）

## 开发流程

### 构建与发布
```bash
# 客户端
cmake --build /c/Users/COZY/Desktop/dou/build

# 服务端
cmake --build /c/Users/COZY/Desktop/dou/build_server

# 构建后必须复制到 dist/
cp build/DoubanQt.exe build_server/ChatServer.exe dist/
```

### 启动测试
- 用 `dist/start.bat` 启动服务端+两个客户端
- debug 输出在启动后的控制台窗口里（qDebug）
- 服务端 HTTP API 可用 curl 测试：`curl http://localhost:8766/api/login -d ...`

### 数据模型规范
- 服务端 JSON 响应必须同时返回 `user_id` + `username`，`from_id` + `from`
- 客户端结构体存 ID 字段，匹配用 ID，显示用名字
- 新增字段时，修改链路：struct → 服务端 JSON → 客户端解析 → 客户端使用

### 通信架构
- 实时推送（聊天消息、上线通知）→ WebSocket :8765
- 请求/响应（登录、好友、影评、头像、资料）→ HTTP :8766
- 头像缓存：`exeDir/cache/avatars/{userId}.png`
- 客户端 `ServerApiClient` 基地址写死在代码里，部署跨电脑时需改
