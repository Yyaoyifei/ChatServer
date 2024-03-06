#include"chatserver.hpp"
#include"json.hpp"
#include<string>
#include<functional>
#include"chatservice.hpp"
using namespace std;
using namespace placeholders;
using json =nlohmann::json;
ChatServer::ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg):_server(loop,listenAddr,nameArg),_loop(loop){
                //注册连接回调·
                _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
                //注册读写回调
                _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
                //设置线程数量
                _server.setThreadNum(4);
            }
void ChatServer::start(){
    _server.start();
}
void ChatServer::onConnection(const TcpConnectionPtr& conn){
  if(!conn->connected()){
    ChatService::instance()->clientCloseException(conn);
  }
}
void ChatServer::onMessage(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp receiveTime){
   if(!conn->connected()){
    ChatService::instance()->clientCloseException(conn);
  }
    string buf=buffer->retrieveAllAsString();
    //数据反序列化
    json js=json::parse(buf);
    //通过js["msgid"]获取一个业务处理器，完全解耦网络模块代码和业务模块代码
    auto handler =ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调相应的代码
    handler(conn,js,receiveTime);
}