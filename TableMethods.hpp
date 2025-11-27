#include <iostream>
#include <deque>
#include "tokens.hpp"

struct MethodSig{
    Token::Type type;
    std::vector<Token::Type> parameters;
};

class MethodTable {
private:
    std::map<std::string, std::map<std::string, std::vector<MethodSig>>> methods;

public:
    void New_method(const std::string& cls, const std::string& name, Token::Type t, const std::vector<Token::Type>& params){
        auto &vec = methods[cls][name];

        for(auto &m:vec){
            if(m.parameters==params)
                throw std::runtime_error("method overload exists: "+cls+"."+name);
        }
        vec.push_back({t,params});
    }

    Token::Type check_call(const std::string& cls,
                           const std::string& name,
                           const std::vector<Token::Type>& params)
    {
        for(auto &m:methods[cls][name]){
            if(m.parameters==params)
                return m.type;
        }
        throw std::runtime_error("class method not found: "+cls+"."+name);
    }
};