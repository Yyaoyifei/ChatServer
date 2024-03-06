#include"json.hpp"
#include<ctime>
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
using namespace std;
using json=nlohmann::json;
#include"group.hpp"
#include"user.hpp"
#include"public.hpp"
#include"groupuser.hpp"
#include<iostream>
#include<thread>
#include<string>
#include<vector>
#include<chrono>
#include<unordered_map>
#include<functional>
#include<iomanip>
//系统支持的用户命令
unordered_map<string,string> commandMap{
    {"help","显示所有支持的命令 格式help"},
    {"chat","一对一聊天 格式 chat:friendid:message"},
    {"addfriend","添加好友,格式addfriend:friendid"},
    {"creategroup","创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup","加入群组,格式addgroup:groupid"},
    {"groupchat","群聊,格式groupchat:groupid:message"},
    {"loginout","注销，格式  loginout"}
};
bool isMainmenuRunning=false;
void help(int fd=0,string arg="");
void chat (int clientfd,string str);

void addfriend(int,string);

void creategroup(int, string);

void addgroup(int,string);

void groupchat(int,string);

void loginout(int,string);
unordered_map<string,function<void(int,string)>> commandHandlerMap={

};


//记录当前登陆的用户信息
User g_currentUser;
//好友信息
vector<User> g_currentUserFriendList;
//群组信息
vector<Group>g_currentUserGroupList;
//显示当前用户基本信息
void showCurrentUserData();
//接收线程
void writeTaskHandler(int clientid){

}
string getCurrentTime(){
  auto now = std::chrono::system_clock::now();
    
    // 将时间转换为time_t类型
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&nowTime);
     char buffer[80];
     std::string format = "%Y-%m-%d %H:%M:%S";
    std::strftime(buffer, 80, format.c_str(), localTime);


    
    // 返回格式化后的时间字符串
    return string(buffer);
}
void readTaskHandler(int clientid){
    while(1){
        char buffer[1024]={0};
        int len=recv(clientid,buffer,1024,0);
        if(-1==len||0==len){
            close(clientid);
            exit(-1);
        }
        json js=json::parse(buffer);
        if(ONE_CHAT_MSG==(js["msgid"].get<int>())){
            cout<<js["time"]<<"["<<js["id"]<<"]"<<js["name"].get<string>()<<"said" <<js["msg"]<<endl;
            continue;
        }else if(GROUP_CHAT_MSG==js["msgid"].get<int>()){
            cout<<"群消息:"<<"["<<js["groupid"]<<"]"<<js["time"]<<"["<<js["id"]<<"]"<<js["name"].get<string>()<<"said" <<js["msg"]<<endl;
        }
        
    }
}
//主聊天界面
void mainMenu(int clientfd){
commandHandlerMap.insert({"help",help});
commandHandlerMap.insert({"addfriend",addfriend});
commandHandlerMap.insert({"chat",chat});
commandHandlerMap.insert({"creategroup",creategroup});
commandHandlerMap.insert({"addgroup",addgroup});
commandHandlerMap.insert({"groupchat",groupchat});
commandHandlerMap.insert({"loginout",loginout});
help();
char buff[1024]={0};
while(isMainmenuRunning){
    cin.getline(buff,1024);
    string commandbuf(buff);
    string command;
    int idx=commandbuf.find(":");
    if(-1!=idx){
        command=commandbuf.substr(0,idx);
    }
    else{
        command=commandbuf;
    }
    auto it =commandHandlerMap.find(command);
    if(it==commandHandlerMap.end()){
         cerr<<"invalid input command!"<<endl;
         continue;
    }
    it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size()-idx));  
}
}
//主线程用作发送线程，子线程用作接收线程
int main(int argc,char**argv){
    if(argc<3){
        cerr<<"command invalid! example: ./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }
    char*ip=argv[1];
    uint16_t port =atoi(argv[2]);
    sockaddr_in server;
    memset(&server,0,sizeof(sockaddr_in));
    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr=inet_addr(ip);
    int clientfd=socket(AF_INET,SOCK_STREAM,0);
    if(clientfd==-1){
        cerr<<"socket create error"<<endl;
    }
    if(-1 ==connect(clientfd,(sockaddr*)&server,sizeof(sockaddr_in))){
        cerr<<"connect server error"<<endl;
        close(clientfd);
        exit(-1);
    }
    for(;;){
        //显示首页菜单 登录 注册 退出
        cout<<"======================="<<endl;
        cout<<"1. login"<<endl;
        cout<<"2. register"<<endl;
        cout<<"3. quit"<<endl;
        cout<<"========================"<<endl;
        cout<<"choice:";
        int choice=0;
        cin>>choice;
        cin.get();//读掉缓冲区残留的回车
        switch(choice){
        case 1:{
            //获取用户id 密码
            int id=0;
            char pwd[50]={0};
            cout<<"userid: ";
            cin>>id;
            cin.get();
            cout<<"password: ";
            cin.getline(pwd,50);
           //封装json
            json js;
            js["msgid"]=LOGIN_MSG;
            js["id"]=id;
            js["password"]=pwd;
            string request=js.dump();
            //发送
            int len =send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
            if(len==-1){
                //发送失败
                cerr<<"send login msg error: "<<request<<endl;
            }
            else{
                char buffer[1024]={0};
                len=recv(clientfd,buffer,1024,0);
                if(-1==len){
                    //接收失败
                    cerr<<"recv login response error"<<endl;
                }else{

                    json responsejson=json::parse(buffer);
                    if(0!=responsejson["errno"].get<int>()){
                           cerr<<responsejson["errmsg"]<<endl;
                    }
                    else{
                        //登陆成功
                        g_currentUser.setId(responsejson["id"].get<int>());
                        g_currentUser.setName(responsejson["name"]); 

                        //记录当前用户好友信息
                        if(responsejson.contains("friends")){
                         g_currentUserGroupList.clear();
                         vector<string> vec=responsejson["friends"];
                         cout<<responsejson["friends"]<<endl;
                         for(string &str:vec){
                            cout<<str;
                            json jsfriend=json::parse(str);
                            cout<<jsfriend<<endl;
                            User user;
                            user.setId(jsfriend["id"].get<int>());
                            user.setName(jsfriend["name"]);
                            user.setState(jsfriend["state"]);
                            g_currentUserFriendList.push_back(user);
                         }
                         cout<<"haha"<<endl;
                        }

                        if(responsejson.contains("groups")){
                            g_currentUserGroupList.clear();
                            vector<string> vec=responsejson["groups"];
                            cout<<"ok"<<endl;
                            for(string &str:vec){
                                json js=json::parse(str);
                                Group group;
                                group.setId(js["id"].get<int>());
                                group.setName(js["groupname"]);
                                group.setdesc(js["groupdesc"]);
                                vector <string> vec2=js["users"];
                                for(string& userstr:vec2){
                                    groupUser user;
                                    json jsusr=json::parse(userstr);
                                    user.setId(jsusr["id"].get<int>());
                                    user.setName(jsusr["name"]);
                                    user.setState(jsusr["state"]);
                                    user.setRole(jsusr["role"]);
                                    group.getUsers().push_back(user);

                                }
                                g_currentUserGroupList.push_back(group);
                            }
                        }
                        showCurrentUserData();
                        if(responsejson.contains("offlinemsg")){
                            vector<string> vec =responsejson["offlinemsg"];
                            for(string&str:vec){
                                json js=json::parse(str);
                                if(ONE_CHAT_MSG==(js["msgid"].get<int>())){
                                   cout<<js["time"]<<"["<<js["id"]<<"]"<<js["name"].get<string>()<<"said" <<js["msg"]<<endl;
                                     }
                                else {
                                     cout<<"群消息:"<<"["<<js["groupid"]<<"]"<<js["time"]<<"["<<js["id"]<<"]"<<js["name"].get<string>()<<"said" <<js["msg"]<<endl;
                                    }
                            }
                        }
                        static int threadnumber=0;
                        if(threadnumber==0){
                            std::thread readTask(readTaskHandler,clientfd);
                            readTask.detach();
                            threadnumber++; 
                        }
                     
                        isMainmenuRunning=true;
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;
        case 2:{
            
            char name[50]={0};
            char pwd[50]={0};
            cout<<"username: ";
            cin.getline(name,50);
            cout<<"password: ";
            cin.getline(pwd,50);
            json js;
            js["msgid"]=REG_MSG;
            js["name"]=name;
            js["password"]=pwd;
            string request=js.dump();
            int len =send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
            if(len==-1){
                cerr<<"send message error!"<<endl;
            }else{
               char buffer[1024]={0};
               len=recv(clientfd,buffer,1024,0);
               if(-1==len){
                cerr<<"recv reg response error"<<endl;
               }else{
                json responsejson=json::parse(buffer);
                if(0!=responsejson["errno"].get<int>()){
                    cerr<<name<<"is already exist,regist error!"<<endl;
                }
                else{
                    cout<<name<<"register success ,userid is "<<responsejson["id"]<<" ,do not forget it!"<<endl;

                }
               }
            }
        }
        break;
        case 3:
           exit(0);
        default:
           cerr<<"invalid input!"<<endl;
           break;
        }
    }


    return 0;
}
void showCurrentUserData(){
cout<<"=================login user=================="<<endl;
cout<<"current login user:"<<g_currentUser.getID()<<" name:"<<g_currentUser.getName()<<endl;
cout<<"--------------------firend list --------------------"<<endl;
if(!g_currentUserFriendList.empty()){
    for(User &user:g_currentUserFriendList){
        cout<<user.getID()<<" "<<user.getName()<<" "<<user.getState()<<endl;

    }
}
cout<<"-------------------group list-------------------"<<endl;
if(!g_currentUserGroupList.empty()){
    for(Group &group:g_currentUserGroupList){
        cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
        for(groupUser &user: group.getUsers()){
            cout<<user.getID()<<" "<<user.getName()<<" "<<user.getState()<<" "<<user.getRole()<<endl;
        }
    }
}
cout<<"================================================"<<endl;
}
//打印所有命令
void help(int fd, string arg){
    cout<<"show command list>>>>"<<endl;
    for(auto &p:commandMap){

        cout<<p.first<<": "<<p.second<<endl;
    }
    cout<<endl;
}
void addfriend(int clientfd,string str)
{
    int friendid=atoi(str.c_str());
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getID();
    js["friendid"]=friendid;
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len){
        cerr<<"send addfriend msg error->"<<buffer<<endl;
    }
}
void chat(int clientfd,string str){
   int idx=str.find(":");
   if(-1==idx){
    cerr<<"chat command invalid!"<<endl;
    return;
   }
   int friendid=atoi(str.substr(0,idx).c_str());
   string message=str.substr(idx+1,str.size()-idx);
   json js;
   js["msgid"]=ONE_CHAT_MSG;
   js["id"]=g_currentUser.getID();
   js["name"]=g_currentUser.getName();
   js["toid"]=friendid;
   js["msg"]=message;
   js["time"]=getCurrentTime();
   string buffer=js.dump();
   int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len){
        cerr<<"send chat msg error->"<<buffer<<endl;
    }
}
void creategroup(int clientfd,string str){
  int idx=str.find(":");
  if(-1==idx){
    cerr<<"create group command invalid!"<<endl;
    return;
  }
  string groupname=str.substr(0,idx);
  string groupdes=str.substr(idx+1,str.size()-idx);
  json js;
  js["msgid"]=CREATE_GROUP_MSG;
  js["id"]=g_currentUser.getID();
  js["groupname"]=groupname;
  js["groupdesc"]=groupdes;
  string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len){
        cerr<<"send create group msg error->"<<buffer<<endl;
    }
}

void addgroup(int clientfd,string str){
int groupid=atoi(str.c_str());
json js;
js["msgid"]=ADD_GROUP_MSG;
js["id"]=g_currentUser.getID();
js["groupid"]=groupid;
  string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len){
        cerr<<"send add group msg error->"<<buffer<<endl;
    }

}

void groupchat(int clientfd,string str){
  int idx=str.find(":");
  if(-1==idx){
    cerr<<" groupchat command invalid!"<<endl;
    return;
  }
  int groupid=atoi(str.substr(0,idx).c_str());
  string message=str.substr(idx+1,str.size()-idx);
  json js;
  js["msgid"]=GROUP_CHAT_MSG;
  js["id"]=g_currentUser.getID();
  js["name"]=g_currentUser.getName();
  js["groupid"]=groupid;
  js["msg"]=message;
  js["time"]=getCurrentTime();
  string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len){
        cerr<<"send group chat msg error->"<<buffer<<endl;
    }
}

void loginout(int clientfd,string str){

json js;
js["msgid"]=LOGINOUT_MSG;
js["id"]=g_currentUser.getID();
  string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len){
        cerr<<"send loginout msg error->"<<buffer<<endl;
    }
    else{
        isMainmenuRunning=false;
    }
}