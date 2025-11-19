#include <bits/stdc++.h>
using namespace std;

class TID{
private:
    map<string,pair<string,string>> nodes;
public:
    TID() : nodes() {}
    void push_id(string name, string type, string value){
        if(nodes.find(name) == nodes.end()){
            throw string("This name already exist");
            return;
        } else{
            nodes[name] = {type, value};
        }
    }
    void del_Tid(){
        nodes.clear();
    }
    auto get_map(){
        return nodes;
    }
};

class treeTID{
private:
    deque<TID> nodes;
public:
    treeTID() : nodes() {}
    void create_TID(){
        TID newTID;
        nodes.push_back(newTID);
    }
    void del_TID(){
        nodes[nodes.size() - 1].del_Tid();
        nodes.pop_back();
    }
    bool check_ID(string name){
        for(int ind = nodes.size() - 1; ind >= 0;--ind){
            if(nodes[ind].get_map().find(name) != nodes[ind].get_map().end()){
                throw string("This identifier already exist in parent number " + to_string(ind));
                return 1;
            }
        }
        return 0;
    }

};