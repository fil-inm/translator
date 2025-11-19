#include <bits/stdc++.h>
using namespace std;

struct Node{
    string name;
    string type;
    int id = 0;
    deque<string> parameters;
};

class TableFunction{
private:
    deque<Node> nodes;

public:
    void New_func(Node newfunc){
        for(auto i : nodes){
            if(i.name == newfunc.name){
                if(i.type == newfunc.type){
                    if(i.parameters == newfunc.parameters){
                        throw string("Same function is already exist");
                    } else{
                        newfunc.id = nodes[nodes.size() - 1].id + 1;
                    }
                }
            }
        }
    }

    string check_call(string name, deque<string> param){
        for(auto i : nodes){
            if(i.name == name && param == i.parameters){
                return i.type;
            }
        }
        throw string("This function is not declared in this scope");
        return "";
    }
};