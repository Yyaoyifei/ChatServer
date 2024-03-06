#ifndef PUBLIC_H
#define PUBLIC_H
/*
server和client的公共文件
*/
enum EnMsgType
{
    LOGIN_MSG=1,//登陆消息
    REG_MSG=2,//注册消息
    REG_MSG_ACK,//注册响应消息
    LOGIN_MSG_ACK,
    ONE_CHAT_MSG,
    ADD_FRIEND_MSG,
    CREATE_GROUP_MSG,//创建群组
    ADD_GROUP_MSG,//加入群组
    GROUP_CHAT_MSG,//群聊
    LOGINOUT_MSG//登出
};
#endif