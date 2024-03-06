#ifndef DB_H
#define DB_H
#include<mysql/mysql.h>
#include<string>
#include<muduo/base/Logging.h>
using namespace std;
//数据库配置信息
//数据库操作类
class MySQL
{
public:
  MySQL();
  ~MySQL();
  bool connect();
  //更新操作
   bool update(string sql);
   //查询操作
   MYSQL_RES * query(string sql);
   MYSQL* getConnection();
private:
 MYSQL* _conn;
};

#endif