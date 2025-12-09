#include <map>
#include <deque>
#include <stdexcept>
#include "tokens.hpp"

class TID {
private:
    std::map<std::string,std::pair<Token::Type,std::string>> nodes;

public:
    void push_id(const std::string& name, Token::Type type, const std::string& value){
        if(nodes.count(name)) throw std::runtime_error("Identifier already declared: " + name);
        nodes[name] = {type,value};
    }
    auto& get() { return nodes; }
};


class treeTID {
private:
    std::deque<TID> nodes;

public:
    void create_TID(){
        nodes.emplace_back();
    }

    void del_TID(){
        if(nodes.empty()) throw std::runtime_error("Scope stack empty");
        nodes.pop_back();
    }

    Token::Type check_id(const std::string& name){
        for(int tid = nodes.size() - 1; tid >= 0; --tid){
            if(!nodes[tid].get().count(name)) throw std::runtime_error("Identifiar is alreade exist");
            return (nodes[tid].get())[name].first;
        }
        throw std::runtime_error("Identifiar is alreade exist");
    }
};
