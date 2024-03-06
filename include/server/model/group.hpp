#ifndef GROUP_H
#define GROUP_H
using namespace std;
#include<string>
#include<vector>
#include"user.hpp"
#include"groupuser.hpp"
class Group{
 public:
    Group(int id=-1,string name="",string desc=""){
        this->id=id;
        this->name=name;
        this->desc=desc;
    }
    void setId(int id){this->id=id;}
    void setName(string name){this->name=name;}
    void setdesc(string desc){this->desc=desc;}
    int getId(){return this->id;}
    string getName(){return this->name;}
    string getDesc(){return this->desc;}
    vector<groupUser> & getUsers(){return this->users;}
    private:
         int id;
         string name;
         string desc;
         vector<groupUser> users;
};







#endif