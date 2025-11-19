#include <bits/stdc++.h>
using namespace std;

struct TItem{
    string name;
    string type;
    int id = 0;
    deque<string> parameters;
};

class TableFunction{
private:
    deque<TItem> nodes;

public:
    void New_func(string name, string type, deque<string> parameters){
        TItem newfunc;
        newfunc.name = name, newfunc.type = type, newfunc.parameters = parameters;
        for(auto i : nodes){
            if(i.name == name){
                if(i.type == type){
                    if(i.parameters == parameters){
                        throw runtime_error("Same function is already exist");
                    } else{
                        newfunc.id = nodes[nodes.size() - 1].id + 1;
                    }
                }
            }
        }
        nodes.push_back(newfunc);
    }

    string check_call(string name, deque<string> parameters){
        for(auto i : nodes){
            if(i.name == name && parameters == i.parameters){
                return i.type;
            }
        }
        throw runtime_error("This function is not declared in this scope");
        return "";
    }
};