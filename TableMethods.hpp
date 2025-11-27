#include "TableFunction.hpp"

class MethodTable {
private:
    std::map<std::string, FuncTable> cam;

public:
    void New_method(const std::string& cls, const std::string& name, const FuncSig& func){
        for(auto meth: cam[cls].get()[name]){
            if(meth.parameters == func.parameters) throw std::runtime_error("The same method is already exist");
        }

        cam[cls].get()[name].push_back(func);
    }

    Token::Type check_call(const std::string& cls, const std::string& name, const std::vector<Token::Type> parameters){
        if(!cam.count(cls)) throw std::runtime_error("Class with this name is not declared");
        if(!cam[cls].get().count(name)) throw std::runtime_error("Method with this name is not declared");

        for(auto meth : cam[cls].get()[name]){
            if(meth.parameters == parameters) return meth.type;
        }

        throw std::runtime_error("This method is not exist");
    }

};