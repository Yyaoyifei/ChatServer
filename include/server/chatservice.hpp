#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include<muduo/net/TcpConnection.h>
//map是为了建立meg和对应业务的处理关系
#include<unordered_map>
#include"usermodel.hpp"
#include"friendmodel.hpp"
#include"offlinemessagemodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"
#include<mutex>
//用functional实现回调机制
#include<functional>
#include<json.hpp>
using json =nlohmann::json;
using namespace muduo;
using namespace muduo::net;
//表示处理事件回调方法实现类型
using MsgHandler =std::function<void(const TcpConnectionPtr& coon,json &js,Timestamp )>;
class ChatService{
public:
//获取单例对象的接口
    static ChatService* instance();
    void login(const TcpConnectionPtr& coon,json &js,Timestamp time);

    void reg(const TcpConnectionPtr& coon,json &js,Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    void clientCloseException(const TcpConnectionPtr&conn);
    void oneChat(const TcpConnectionPtr& coon,json &js,Timestamp time);
    void reset();
    void addFriend(const TcpConnectionPtr& coon,json &js,Timestamp time);
    void createGroup(const TcpConnectionPtr& coon,json &js,Timestamp time);
    void addGroup(const TcpConnectionPtr& coon,json &js,Timestamp time);
    void groupChat(const TcpConnectionPtr& coon,json &js,Timestamp time);
    void loginout(const TcpConnectionPtr& coon,json &js,Timestamp time);
    void handlerRedisSubscribeMessage(int userid,string message);
private:
    ChatService();
    //存储消息id和其对应的事件处理方法映射
    std::unordered_map<int,MsgHandler> _msgHandlerMap;
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
    //记录用户连接信息
    unordered_map<int,TcpConnectionPtr> _userConnMap; 
    //保证map的线程安全
    mutex _Connmutex;
    Redis _redis;
};




#endif