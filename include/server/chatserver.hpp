#ifndef CHATSERVER_H
#define CHATSERVER_H
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer{

public:
    //初始化服务器
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);
    void start();
private:
//连接回调函数
void onConnection(const TcpConnectionPtr& conn);
//读写时间的回调
void onMessage(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp receiveTime);
TcpServer _server;//服务器功能类对象
EventLoop * _loop;//事件循环指针对象

};


#endif