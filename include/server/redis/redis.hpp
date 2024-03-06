#ifndef REDIS_H
#define REDIS_H
#include<hiredis/hiredis.h>
#include<thread>
#include<functional>
using namespace std;
class Redis{
public:
  Redis();
  ~Redis();

  bool connect();

  bool publish(int channel,string message);
   
   bool  subscribe(int channel);

   bool unsubscribe(int channel);
   //在独立线程中接收订阅通道中的消息
   void observer_channel_message();
  //初始化向业务层上报消息的回调对象
   void init_notify_handler(function<void(int,string)> func);
  






private:
  //hi rediss 同步上下文对象，一个负责publish，一个负责subsc
   redisContext * _publish_context;
   redisContext * _subscribe_contex;
   //回调操作
    function<void(int,string)> _notify_message_handler;



};

#endif