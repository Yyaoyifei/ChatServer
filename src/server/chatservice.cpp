#include"chatservice.hpp"
#include"public.hpp"
#include<functional>
#include<vector>
#include<muduo/base/Logging.h>
#include<iostream>
using namespace std;
/*
负载均衡器

1.把client的请求按照负载算法分发到具体的业务服务器上

2.能够和ChatServer保持心跳机制，检测ChatServer故障

3.能够发现新的ChatServer设备，扩展服务器数量

LVS 带多台nginx

选择nginx的tcp负载均衡模块

1.如何进行nginx源码编译，包含负载均衡模块

2.nginx.conf如何配置负载均衡

3.nginx的平滑加载配置文件启动

2.如何解决跨服务器通信问题

_userConnMap

服务器中间件（基于redis发布订阅的消息队列）

kafka

基于发布-订阅的消息队列    观察者设计模式

每个客户端订阅

nginx默认没有编译tcp负载均衡，编译时需要加入--with-stream激活这个模块

./nginx -s reload

./nginx -s stop


*/
ChatService* ChatService::instance(){
    static ChatService chatService;
    return &chatService;
}
ChatService::ChatService(){
   _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
   _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
   _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,_1,_2,_3)});
    if(_redis.connect()){
      //设置上报消息的回调
      _redis.init_notify_handler(std::bind(&ChatService::handlerRedisSubscribeMessage,this,_1,_2));
    }
}
void ChatService::login(const TcpConnectionPtr& coon,json &js,Timestamp time){
 int id=js["id"];
 string pwd=js["password"];

 User user=_userModel.query(id);
 if(user.getID()==id&&user.getPwd()==pwd){
    if(user.getState()=="online"){
        json response;
    response["msgid"]=LOGIN_MSG_ACK;
    response["errno"]=2;
    response["errmsg"]="该账号已经登陆，请重新输入账号";
    coon->send(response.dump());
    }
else{
  //登录成功，更新用户状态信息
  {lock_guard<mutex> lock(_Connmutex);
   _userConnMap.insert({id,coon});}//线程安全
   user.setState("online");
    _userModel.updateState(user);
    json response;
    response["msgid"]=LOGIN_MSG_ACK;
    response["errno"]=0;
    response["id"]=user.getID();
    response["name"]=user.getName();
    //查询该用户是否有离线消息
    vector<string> vec=_offlineMsgModel.query(id);
    if(!vec.empty()){
        response["offlinemsg"]=vec;
        _offlineMsgModel.remove(id);
    }
    vector<User> userVec= _friendModel.query(id);
    if(!userVec.empty()){
        vector<string> vec2;
        for(User &user:userVec){
          json jsfriend;
          jsfriend["id"]=user.getID();
          jsfriend["name"]=user.getName();
          jsfriend["state"]=user.getState();
          vec2.push_back(jsfriend.dump());
        }
        response["friends"]=vec2;
    }
    vector<Group> groupVec=_groupModel.queryGroup(id);
    if(!groupVec.empty()){
      vector<string> vec3;
       for(Group & group:groupVec){
          json jsGroup;
          jsGroup["id"]=group.getId();
          jsGroup["groupname"]=group.getName();
          jsGroup["groupdesc"]=group.getDesc();
          vector<string> usersVec;
          for(groupUser &groupUser: group.getUsers()){
            json jsUsers;
            jsUsers["id"]=groupUser.getID();
            jsUsers["name"]=groupUser.getName();
            jsUsers["state"]=groupUser.getState();
            jsUsers["role"]=groupUser.getRole();
            usersVec.push_back(jsUsers.dump());
          }
          jsGroup["users"]=usersVec;
          vec3.push_back(jsGroup.dump());
       }

       response["groups"]=vec3;
    }
    
    coon->send(response.dump());
    _redis.subscribe(id);
}

 }else{
  //该用户不存在，或者用户存在或者密码错误登录失败
json response;
    response["msgid"]=LOGIN_MSG_ACK;
    response["errno"]=0;
    coon->send(response.dump());

 }


}
//name password
void ChatService::reg(const TcpConnectionPtr& coon,json &js,Timestamp time){
  string name=js["name"];
  string password=js["password"];
  User user;
  user.setName(name);
  user.setPwd(password);
  bool state =_userModel.insert(user);
  if(state){
    //注册成功
    json response;
    response["msgid"]=REG_MSG_ACK;
    response["errno"]=0;
    response["id"]=user.getID();
    coon->send(response.dump());

  }else{
  //注册失败
   json response;
    response["msgid"]=REG_MSG_ACK;
    response["errno"]=1;
    coon->send(response.dump());
  }
 }
 MsgHandler ChatService::getHandler(int msgid){
    //记录错误日志
    auto it =_msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end()){
         return [=](const TcpConnectionPtr& coon,json &js,Timestamp ){
             LOG_ERROR<<"msgid:"<<msgid<<"can not find handler!";
         };
    }
    else{
        return _msgHandlerMap[msgid]; 
    }
 }
 //意外退出业务逻辑
 void ChatService::clientCloseException(const TcpConnectionPtr & conn){
  
  User user;
 {lock_guard<mutex> lock(_Connmutex);
    for(auto it=_userConnMap.begin();it!=_userConnMap.end();++it){
    if(it->second==conn)
    {  
        user.setId(it->first);
           //从map表删除
        _userConnMap.erase(it);
           break;

    }
  }
  _redis.unsubscribe(user.getID());
 }
 user.setState("offline");
  _userModel.updateState(user);
  
 }
 void ChatService::oneChat(const TcpConnectionPtr& coon,json &js,Timestamp time){
  int  toid=js["toid"].get<int>();

  {
    lock_guard<mutex> lock(_Connmutex);
    auto it =_userConnMap.find(toid);
    if(it!=_userConnMap.end()){
     //转发消息
      it->second->send(js.dump());
      return;
    }
  
  }
  User user =_userModel.query(toid);
  if(user.getState()=="online"){
    _redis.publish(toid,js.dump());
    return;
  }
  _offlineMsgModel.insert(toid,js.dump());
  //toid不在线，存储离线消息
 }
 void ChatService::reset(){
//把online状态用户设置为offline
_userModel.resetState();
 }
  void ChatService::addFriend(const TcpConnectionPtr& coon,json &js,Timestamp time){
   int userid=js["id"].get<int>();
   int friendid=js["friendid"].get<int>();
   _friendModel.insert(userid,friendid);
  }
void ChatService::createGroup(const TcpConnectionPtr& coon,json &js,Timestamp time){
  int userid=js["id"].get<int>();
  string name=js["groupname"];
  string desc=js["groupdesc"];
  Group group(-1,name,desc);
  //创建群组成功
  if(_groupModel.CreateGroup(group)){
    _groupModel.addGroup(userid,group.getId(),"creator");
  }

}
//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& coon,json &js,Timestamp time){
int userid=js["id"].get<int>();
int groupid=js["groupid"].get<int>();
_groupModel.addGroup(userid,groupid,"normal");
}
//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr& coon,json &js,Timestamp time){
int userid=js["id"].get<int>();
int groupid=js["groupid"].get<int>();
vector<int> vec=_groupModel.queryGroupUsers(userid,groupid);
lock_guard<mutex>lock(_Connmutex);
for(int id:vec){
  auto it=_userConnMap.find(id);
  if(it!=_userConnMap.end()){
    it->second->send(js.dump());
  }
  else{
    //查看是否在线
    User user=_userModel.query(id);
    if(user.getState()=="online"){
      _redis.publish(id,js.dump());
    }
    else{
       _offlineMsgModel.insert(id,js.dump());
    }
    
  }
}
}
void ChatService::loginout(const TcpConnectionPtr& coon,json &js,Timestamp time){
  int userid=js["id"].get<int>();
  {
    lock_guard<mutex> lock(_Connmutex);
    auto it=_userConnMap.find(userid);
    if(it!=_userConnMap.end()){
      _userConnMap.erase(it);
    }
  }
  User user(userid,"","","offline");
  _redis.unsubscribe(userid);
  _userModel.updateState(user);
  
}
void ChatService::handlerRedisSubscribeMessage(int userid,string messsage){
 lock_guard<mutex> lock(_Connmutex);
 auto it=_userConnMap.find(userid);
 if(it!=_userConnMap.end()){
  it->second->send(messsage);
  return ;
 }
 _offlineMsgModel.insert(userid,messsage);
}