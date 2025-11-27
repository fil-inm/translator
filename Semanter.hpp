#include "tokens.hpp"
#include "treeTID.hpp"
#include "TableFunction.hpp"
#include "TableMethods.hpp"
#include "TAO.hpp"

class Semanter {
private:
    treeTID vars;
    FuncTable funcs;
    MethodTable methods;
    SemStack estack;

    Token::Type currentFuncRet = Token::Type::KwVoid;

public:
    Semanter() {
        vars.create_TID();
    }

    void enterScope(){ vars.create_TID(); }
    void leaveScope(){ vars.del_TID(); }

    void declareVar(const std::string& name, Token::Type t){
        vars.push_id(name,t,"");
    }

    Token::Type getVar(const std::string& name){
        return vars.lookup(name);
    }

    void declareFunc(const std::string& name,
                     Token::Type ret,
                     const std::vector<Token::Type>& params)
    {
        funcs.New_func(name,ret,params);
    }

    Token::Type callFunc(const std::string& name,
                         const std::vector<Token::Type>& params)
    {
        return funcs.check_call(name,params);
    }

    void declareMethod(const std::string& cls,
                       const std::string& name,
                       Token::Type ret,
                       const std::vector<Token::Type>& params)
    {
        methods.New_method(cls,name,ret,params);
    }

    Token::Type callMethod(const std::string& cls,
                           const std::string& name,
                           const std::vector<Token::Type>& params)
    {
        return methods.check_call(cls,name,params);
    }

    void pushExpr(Token::Type t){
        estack.push(t,"");
    }

    void pushLiteral(Token::Type t, const std::string& v){
        estack.push(t,v);
    }

    Token::Type unary(Token::Type op){
        return estack.check_unary();
    }

    Token::Type binary(Token::Type op){
        return estack.check_bin();
    }

    void setCurrentFunc(Token::Type t){
        currentFuncRet = t;
    }

    Token::Type getCurrentFunc(){
        return currentFuncRet;
    }
};
