#include"redis.hpp"
#include<iostream>
using namespace std;
  Redis::Redis():_publish_context(nullptr),_subscribe_contex(nullptr)
  {

  }
  Redis::~Redis(){
    if(_publish_context!=nullptr){
        redisFree(_publish_context);
    }
    if(_subscribe_contex!=nullptr){
        redisFree(_subscribe_contex);
    }
  }

  bool Redis::connect(){
    _publish_context=redisConnect("127.0.0.1",6379);
    if(nullptr==_publish_context){
        cerr<<"connect redis failed!"<<endl;
        return false;
    }
    _subscribe_contex=redisConnect("127.0.0.1",6379);
      if(nullptr==_subscribe_contex){
        cerr<<"connect redis failed!"<<endl;
        return false; 
    }
    //在单独的线程中，监听通道上的事件
    thread t([&](){
        observer_channel_message();
    });
    t.detach();
    cout<< "connect redis-erver success!"<<endl;
    return true;
  }

  bool Redis::publish(int channel,string message){
    redisReply * reply=(redisReply*)redisCommand(_publish_context,"PUBLISH %d %s",channel,message);
    if(nullptr==reply){
        cerr<<"publish command false"<<endl;
        return false;
    }
    freeReplyObject(reply);
    cout<<"我发步了"<<endl;
    return true;
  }
   
   bool  Redis::subscribe(int channel){
    //subscribe会阻塞，这里制作订阅通道，不接受通道消息
    //通道西澳西接收在独立线程中进行
    //只负责发送命令，不阻塞接收redis server 响应消息
    if(REDIS_ERR ==redisAppendCommand(this->_subscribe_contex,"SUBSCRIBE %d",channel)){
        cerr<<"subscribe command failed!"<<endl;
        return false;
    }
    int done =0;
    while(!done){
        if(REDIS_ERR==redisBufferWrite(this->_subscribe_contex,&done)){
            cerr<<"subscribe command failed!"<<endl;
            return false;
        }
    }
    cout<<"我订阅了！"<<endl;
    return true;

   }

   bool Redis::unsubscribe(int channel){
       if(REDIS_ERR ==redisAppendCommand(this->_subscribe_contex,"UNSUBSCRIBE %d",channel)){
        cerr<<"unsubscribe command failed!"<<endl;
        return false;
    }
    int done =0;
    while(!done){
        if(REDIS_ERR==redisBufferWrite(this->_subscribe_contex,&done)){
            cerr<<"unsubscribe command failed!"<<endl;
            return false;
        }
    }
    cout<<"我不订阅了"<<endl;
    return true;}

   //在独立线程中接收订阅通道中的消息
   void Redis::observer_channel_message(){
    cout<<"有消息"<<endl;
    redisReply *reply=nullptr;
    while(REDIS_OK==redisGetReply(this->_subscribe_contex,(void**) &reply)){
        //订阅收到的消息是一个带三元素的数组

        if(reply!=nullptr&& reply->element[2]!=nullptr&&reply->element[2]->str!=nullptr){
            //给业务层上报
            _notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr<<"-------------observe channel mesage quit----------------"<<endl;
   }
  //初始化向业务层上报消息的回调对象
   void Redis::init_notify_handler(function<void(int,string)> func){
    this->_notify_message_handler=func;
   }