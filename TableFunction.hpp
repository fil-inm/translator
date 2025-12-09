#include "tokens.hpp"

struct FuncSig {
    Token::Type type;
    std::vector<Token::Type> parameters;

    const bool operator==(const FuncSig& rhs){
        if(type == rhs.type && parameters == rhs.parameters) return 1;
        return 0;
    }
};

class FuncTable {
private:
    std::map<std::string,std::vector<FuncSig>> funcs;

public:
    void New_func(const std::string& name, const FuncSig& info){
        for(auto& vecfunc : funcs){
            for(auto& func : vecfunc.second){
                if(func == info){
                    throw std::runtime_error("The same function is already exist");
                }
            }
        }

        funcs[name].push_back(info);
    }

    Token::Type check_call(const std::string& name, std::vector<Token::Type> parameters){
        if(!funcs.count(name)) throw std::runtime_error("This name of function is not declared");

        for(auto func : funcs[name]){
            if(func.parameters == parameters) return func.type;
        }

        throw std::runtime_error("This list of parameters is not exist");
    }

    auto& get(){ return funcs; }
};
 