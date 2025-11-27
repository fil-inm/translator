#include "tokens.hpp"

struct FuncSig {
    Token::Type type;
    std::vector<Token::Type> parameters;
};

class FuncTable {
private:
    std::map<std::string,std::vector<FuncSig>> funcs;

public:
    void New_func(const std::string& name,
                  Token::Type type,
                  const std::vector<Token::Type>& params)
    {
        for(auto &f : funcs[name]){
            if(f.parameters == params)
                throw std::runtime_error("function overload exists: "+name);
        }
        funcs[name].push_back({type,params});
    }

    Token::Type check_call(const std::string& name,
                           const std::vector<Token::Type>& params)
    {
        for(auto &f : funcs[name]){
            if(f.parameters == params)
                return f.type;
        }
        throw std::runtime_error("function not declared: "+name);
    }
};
