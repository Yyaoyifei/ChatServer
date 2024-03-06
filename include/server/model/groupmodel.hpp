#ifndef GROUPMODEL_H
#define GROUPMODEL_H
#include"group.hpp"
#include<vector>
#include<string>
using namespace std;
class GroupModel
{
public:
   bool CreateGroup(Group &group);
   void addGroup(int userid,int groupid,string role);
   vector<Group> queryGroup(int userid);
   vector<int> queryGroupUsers(int userid,int groupid);


};

#endif