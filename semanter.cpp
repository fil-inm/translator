#include "semanter.hpp"

bool FunctionSignature::matches(const std::vector<TypeInfo>& args,
                                const Semanter& sem) const {
    if (args.size() != params.size())
        return false;

    for (size_t i = 0; i < args.size(); ++i)
        if (!sem.compatible(params[i], args[i]))
            return false;

    return true;
}


Semanter::Semanter() {
    enterScope();
}


void Semanter::enterScope() {
    scopes.emplace_back();
}

void Semanter::leaveScope() {
    if (scopes.empty())
        throw std::runtime_error("Internal error: leaveScope on empty scope stack");
    scopes.pop_back();
}

void Semanter::enterFunctionScope(const TypeInfo& ret) {
    scopes.clear();
    nextSlot = 0;
    currentReturn = ret;
    enterScope();
}


void Semanter::declareVariable(const std::string& name, const TypeInfo& type) {
    auto& scope = scopes.back();
    if (scope.contains(name))
        throw std::runtime_error("Variable '" + name + "' already declared");

    scope[name] = Symbol{name, type, nextSlot++};
}

Symbol* Semanter::lookupVariable(const std::string& name) {
    for (int i = scopes.size() - 1; i >= 0; --i) {
        auto it = scopes[i].find(name);
        if (it != scopes[i].end())
            return &it->second;
    }
    return nullptr;
}


FunctionSymbol* Semanter::declareFunction(
    const std::string& name,
    const TypeInfo& ret,
    const std::vector<TypeInfo>& params
) {
    auto& vec = functions[name];

    for (auto& f : vec)
        if (f.sig.returnType == ret && f.sig.params == params)
            throw std::runtime_error("Function already declared: " + name);

    FunctionSymbol f;
    f.sig = {name, params, ret};
    f.declared = true;
    vec.push_back(f);
    return &vec.back();
}

FunctionSymbol* Semanter::defineFunction(
    const std::string& name,
    const TypeInfo& ret,
    const std::vector<TypeInfo>& params
) {
    auto& vec = functions[name];

    for (auto& f : vec) {
        if (f.sig.returnType == ret && f.sig.params == params) {
            if (!f.declared)
                throw std::runtime_error("Function not declared: " + name);
            if (f.defined)
                throw std::runtime_error("Function already defined: " + name);
            f.defined = true;
            return &f;
        }
    }

    throw std::runtime_error("No matching declaration for function: " + name);
}

FunctionSymbol* Semanter::resolveFunction(
    const std::string& name,
    const std::vector<TypeInfo>& args
) {
    if (!functions.contains(name))
        throw std::runtime_error("Unknown function: " + name);

    FunctionSymbol* best = nullptr;

    for (auto& f : functions[name]) {
        if (f.sig.matches(args, *this)) {
            if (best)
                throw std::runtime_error("Ambiguous overload for function: " + name);
            best = &f;
        }
    }

    if (!best)
        throw std::runtime_error("No matching overload for function: " + name);

    return best;
}


void Semanter::beginFunctionCall(const std::string& name) {
    callStack.push({name, {}});
}

void Semanter::addCallArg() {
    if (callStack.empty())
        throw std::runtime_error("No active function call");
    callStack.top().args.push_back(popType());
}

FunctionSymbol* Semanter::endFunctionCall() {
    auto ctx = callStack.top();
    callStack.pop();

    std::reverse(ctx.args.begin(), ctx.args.end());

    auto* f = resolveFunction(ctx.name, ctx.args);
    pushType(f->sig.returnType);
    return f;
}


void Semanter::pushType(const TypeInfo& t) {
    typeStack.push(t);
}

TypeInfo Semanter::popType() {
    if (typeStack.empty())
        throw std::runtime_error("Type stack underflow");
    auto t = typeStack.top();
    typeStack.pop();
    return t;
}

TypeInfo Semanter::peekType() const {
    if (typeStack.empty())
        throw std::runtime_error("Type stack underflow (peek)");
    return typeStack.top();
}


bool Semanter::compatible(const TypeInfo& dst, const TypeInfo& src) const {
    if (dst == src) return true;
    if (src.baseType == Token::Type::KwChar && dst.baseType == Token::Type::KwInt)
        return true;
    return false;
}

void Semanter::checkAssignment(const TypeInfo& left, const TypeInfo& right) {
    if (!compatible(left, right))
        throw std::runtime_error("Incompatible assignment");
    pushType(left);
}

void Semanter::checkReturn(const TypeInfo& expected, const TypeInfo& actual) {
    if (!compatible(expected, actual))
        throw std::runtime_error("Invalid return type");
}

void Semanter::checkIfCondition(const TypeInfo& t) {
    if (!t.isBool() && !t.isIntegral())
        throw std::runtime_error("Invalid if condition");
}

TypeInfo Semanter::commonNumeric(const TypeInfo& a, const TypeInfo& b) const {
    if (a.baseType == Token::Type::KwFloat || b.baseType == Token::Type::KwFloat)
        return TypeInfo{Token::Type::KwFloat};
    return TypeInfo{Token::Type::KwInt};
}

void Semanter::checkUnaryOp(Token::Type op) {
    auto t = popType();
    if (!t.isNumeric())
        throw std::runtime_error("Unary op on non-numeric");
    pushType(t);
}

void Semanter::checkBinaryOp(Token::Type op) {
    auto b = popType();
    auto a = popType();

    if (!a.isNumeric() || !b.isNumeric())
        throw std::runtime_error("Binary op on non-numeric");

    pushType(commonNumeric(a, b));
}

void Semanter::declareArray(const std::string& name,
                            const TypeInfo& elemType,
                            int size) {
    auto& scope = scopes.back();
    if (scope.contains(name))
        throw std::runtime_error("Variable '" + name + "' already declared");

    TypeInfo arr = TypeInfo::makeArray(elemType, size);
    scope[name] = Symbol{name, arr, nextSlot++};
}

void Semanter::checkArrayIndex(const TypeInfo& arr,
                               const TypeInfo& idx) const {
    if (!arr.isArray)
        throw std::runtime_error("Indexing non-array");

    if (!idx.isIntegral())
        throw std::runtime_error("Array index must be integer");
}

TypeInfo Semanter::getLiteralType(const Token& tok) const {
    switch (tok.type) {
        case Token::Type::IntegerLiteral: return TypeInfo(Token::Type::KwInt);
        case Token::Type::FloatLiteral:   return TypeInfo(Token::Type::KwFloat);
        case Token::Type::CharLiteral:    return TypeInfo(Token::Type::KwChar);
        case Token::Type::StringLiteral:  return TypeInfo(Token::Type::KwString);
        case Token::Type::KwTrue:
        case Token::Type::KwFalse:         return TypeInfo(Token::Type::KwBool);
        default:
            throw std::runtime_error("Unknown literal type");
    }
}

void Semanter::checkPrint(const TypeInfo& t) const {
    if (t.isVoid())
        throw std::runtime_error("Cannot print void");
}

void Semanter::checkRead(const TypeInfo& t) const {
    if (t.isVoid())
        throw std::runtime_error("read(): cannot read into void");

    if (!t.isNumeric() && !t.isBool() && !t.isChar())
        throw std::runtime_error("read(): unsupported type");
}

