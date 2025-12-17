#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <optional>
#include <stdexcept>

#include "typeinfo.hpp"
#include "tokens.hpp"


struct Symbol {
    std::string name;
    TypeInfo type;
    int slot;
};

class Semanter;



struct FunctionSignature {
    std::string name;
    std::vector<TypeInfo> params;
    TypeInfo returnType;

    bool matches(const std::vector<TypeInfo>& args, const Semanter& sem) const;
};

struct FunctionSymbol {
    FunctionSignature sig;
    bool declared = false;
    bool defined = false;
    int entryIp = -1;
    int polizIndex = -1;
};


struct CallContext {
    std::string name;
    std::vector<TypeInfo> args;
};


class Semanter {
public:
    Semanter();

    void enterScope();
    void leaveScope();

    void enterFunctionScope(const TypeInfo &ret);

    void declareVariable(const std::string& name, const TypeInfo& type);
    Symbol* lookupVariable(const std::string& name);

    FunctionSymbol* declareFunction(
    const std::string& name,
    const TypeInfo& ret,
    const std::vector<TypeInfo>& params
);

    FunctionSymbol* defineFunction(
        const std::string& name,
        const TypeInfo& ret,
        const std::vector<TypeInfo>& params
    );

    FunctionSymbol* resolveFunction(
        const std::string& name,
        const std::vector<TypeInfo>& args
    );

    void beginFunctionCall(const std::string& name);
    void addCallArg();
    FunctionSymbol* endFunctionCall();

    void pushType(const TypeInfo& t);
    TypeInfo popType();
    TypeInfo peekType() const;

    void checkAssignment(const TypeInfo& left, const TypeInfo& right);
    void checkReturn(const TypeInfo& expected, const TypeInfo& actual);
    void checkIfCondition(const TypeInfo& t);

    void checkUnaryOp(Token::Type op);
    void checkBinaryOp(Token::Type op);

    TypeInfo commonNumeric(const TypeInfo& a, const TypeInfo& b) const;
    bool compatible(const TypeInfo& dst, const TypeInfo& src) const;

    void declareArray(const std::string& name, const TypeInfo& elemType, int size);

    void checkArrayIndex(const TypeInfo& arr, const TypeInfo& idx) const;

    TypeInfo getLiteralType(const Token& tok) const;

    void checkPrint(const TypeInfo& t) const;

    const TypeInfo& currentReturnType() const {
        return currentReturn;
    }

    void checkRead(const TypeInfo& t) const;

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    int nextSlot = 0;

    std::unordered_map<std::string, std::vector<FunctionSymbol>> functions;

    std::stack<TypeInfo> typeStack;

    std::stack<CallContext> callStack;

    TypeInfo currentReturn;
};