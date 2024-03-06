#ifndef GROUPUSER_H
#define GROUPUSER_H
#include"user.hpp"
class  groupUser: public User
{
private:
   string role;
public:
    void setRole(string role){this->role=role;}
    string getRole(){return this->role;}
};
#endif