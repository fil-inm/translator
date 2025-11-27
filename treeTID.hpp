#pragma once
#include <map>
#include <deque>
#include <stdexcept>
#include "tokens.hpp"

class TID {
private:
    std::map<std::string,std::pair<Token::Type,std::string>> nodes;

public:
    void push_id(const std::string& name, Token::Type type, const std::string& value){
        if(nodes.count(name)) throw std::runtime_error("identifier already declared: " + name);
        nodes[name] = {type,value};
    }

    bool contains(const std::string& name) const{
        return nodes.count(name);
    }

    Token::Type check_id(const std::string& name){
        if(contains(name)) return nodes[name].first;
        throw std::runtime_error("This variable is not declared in this scope");
    }

    const auto& get() const { return nodes; }
};


class treeTID {
private:
    std::deque<TID> nodes;

public:
    void create_TID(){
        nodes.emplace_back();
    }

    void del_TID(){
        if(nodes.empty())
            throw std::runtime_error("scope stack empty");
        nodes.pop_back();
    }

    void push_id(const std::string& name, Token::Type t, const std::string& val){
        if(nodes.empty())
            throw std::runtime_error("scope stack empty");
        nodes.back().push_id(name,t,val);
    }

    Token::Type lookup(const std::string& name){
        for(int i = nodes.size()-1; i>=0; i--){
            if(nodes[i].contains(name))
                return nodes[i].get().at(name).first;
        }
        throw std::runtime_error("variable not declared: "+name);
    }
};
