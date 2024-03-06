#ifndef USERMODEL_H
#define USERMODEL_H
#include"user.hpp"
class UserModel
{
private:
    /* data */
public:
   bool insert(User &user);
   //根据用户号码查询用户id
   User query(int id);
   bool updateState(User user);
   void resetState();
};



#endif