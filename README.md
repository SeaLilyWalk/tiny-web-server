# README

该项目是一个小型的高并发web服务器，能够应用HTTP/Websockets实现一个即时的网络聊天室
计算WebSocketAccept值时使用了来自 https://github.com/vog/sha1 的sha1cpp



## 核心技术
1. epoll
2. 多线程（之后再加
3. 解决粘包/半包问题



## 代码结构
1. main.cc

2. server.h
   用于处理与客户端的连接

3. connection.h

4. hhh