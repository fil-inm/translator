#include "semanter.hpp"
#include <stdexcept>
#include <iostream>


Semanter::Semanter() : scopeLevel(0), currentClass("") {
    enterScope();
}

void Semanter::enterScope() {
    ++scopeLevel;
    tidStack.push_back(std::make_unique<TID>());
}

void Semanter::leaveScope() {
    if (!tidStack.empty()) {
        tidStack.pop_back();
        --scopeLevel;
    }
}

void Semanter::enterFunctionScope() {
    scopeLevel = 0;
    nextSlot = 0;
    tidStack.clear();
    tidStack.push_back(std::make_unique<TID>());
}

TID *Semanter::currentTID() {
    return tidStack.empty() ? nullptr : tidStack.back().get();
}

bool Semanter::declareVariable(const std::string &name,
                               const TypeInfo &type,
                               bool isConst) {
    if (tidStack.empty())
        return false;

    int slot = nextSlot++;

    if (!tidStack.back()->insert(name, type, isConst, slot)) {
        throw std::runtime_error(
            "Переменная '" + name + "' уже объявлена"
        );
    }
    return true;
}

bool Semanter::declareArray(const std::string &name, const TypeInfo &t, int size) {
    if (size <= 0 && size != -1)
        throw std::runtime_error("Размер массива должен быть >0 или -1");
    TypeInfo arr = TypeInfo::makeArray(t, size);
    return declareVariable(name, arr);
}

SymbolEntry *Semanter::lookupVariable(const std::string &name) {
    for (int i = static_cast<int>(tidStack.size()) - 1; i >= 0; --i) {
        SymbolEntry *s = tidStack[i]->find(name);
        if (s) return s;
    }

    if (!currentClass.empty()) {
        ClassEntry *c = lookupClass(currentClass);
        if (!c)
            throw std::runtime_error("Internal error: currentClass установлен, но класс не найден");
        auto it = c->fields.find(name);
        if (it != c->fields.end())
            return &it->second;
    }

    return nullptr;
}


void Semanter::pushType(const TypeInfo &t) {
    typeStack.push(t);
}

TypeInfo Semanter::popType() {
    if (typeStack.empty())
        throw std::runtime_error("Internal error: popType на пустом стеке");
    TypeInfo t = typeStack.top();
    typeStack.pop();
    return t;
}

TypeInfo Semanter::peekType() const {
    if (typeStack.empty())
        throw std::runtime_error("Internal error: peekType на пустом стеке");
    return typeStack.top();
}

bool Semanter::typeEmpty() const {
    return typeStack.empty();
}


bool Semanter::declareClass(const std::string &name) {
    if (classes.contains(name))
        throw std::runtime_error("Класс '" + name + "' уже объявлен");
    classes.emplace(name, ClassEntry(name));
    return true;
}

bool Semanter::addClassField(const std::string &cn, const std::string &field, const TypeInfo &t) {
    ClassEntry *c = lookupClass(cn);
    if (!c)
        throw std::runtime_error("Класс '" + cn + "' не найден");

    if (c->fields.contains(field))
        throw std::runtime_error("Поле '" + field + "' уже существует в классе " + cn);

    c->fields.emplace(
        field,
        SymbolEntry(field, t, false, /*slot*/ -1));
    return true;
}

bool Semanter::addClassMethod(const std::string &cn,
                              const std::string &m,
                              const std::string &uniq,
                              const TypeInfo &rt) {
    ClassEntry *c = lookupClass(cn);
    if (!c)
        throw std::runtime_error("Класс '" + cn + "' не найден");
    c->methods[m].push_back(MethodEntry(cn, m, uniq, rt));
    return true;
}

bool Semanter::addConstructor(const std::string &cn, const std::string &uniq) {
    ClassEntry *c = lookupClass(cn);
    if (!c)
        throw std::runtime_error("Класс '" + cn + "' не найден");
    c->constructors.push_back(uniq);
    return true;
}

ClassEntry *Semanter::lookupClass(const std::string &name) {
    auto it = classes.find(name);
    return (it == classes.end() ? nullptr : &it->second);
}

void Semanter::setCurrentClass(const std::string &cn) {
    currentClass = cn;
}

void Semanter::clearCurrentClass() {
    currentClass.clear();
}


bool Semanter::declareFunction(const std::string &name,
                               const std::string &uniq,
                               const TypeInfo &rt) {
    functions[name].push_back(FunctionEntry(name, uniq, rt));
    return true;
}

bool Semanter::addFunctionParam(const std::string &fname,
                                const std::string &pname,
                                const TypeInfo &t) {
    auto &vec = functions[fname];
    if (vec.empty())
        throw std::runtime_error("Функция '" + fname + "' не найдена при добавлении параметра");
    vec.back().params.push_back(ParamInfo{pname, t});
    return true;
}

FunctionEntry *Semanter::resolveFunction(const std::string &name,
                                         const std::vector<TypeInfo> &args) {
    if (!functions.contains(name))
        throw std::runtime_error("Функция '" + name + "' не найдена");

    auto &overloads = functions[name];
    for (auto &f: overloads) {
        if (f.matchSignature(args))
            return &f;
    }

    throw std::runtime_error("Нет подходящей перегрузки функции '" + name + "'");
}


bool Semanter::addMethodParam(const std::string &cn,
                              const std::string &m,
                              const TypeInfo &t) {
    ClassEntry *c = lookupClass(cn);
    if (!c)
        throw std::runtime_error("Класс '" + cn + "' не найден");

    auto &overloads = c->methods[m];
    if (overloads.empty())
        throw std::runtime_error("Метод '" + m + "' не найден в классе " + cn);

    overloads.back().params.push_back(ParamInfo{"", t});
    return true;
}

MethodEntry *Semanter::resolveMethod(const std::string &cn,
                                     const std::string &name,
                                     const std::vector<TypeInfo> &args) {
    ClassEntry *c = lookupClass(cn);
    if (!c)
        throw std::runtime_error("Класс '" + cn + "' не найден");

    if (!c->methods.contains(name))
        throw std::runtime_error("Метод '" + name + "' не найден в классе '" + cn + "'");

    auto &overloads = c->methods[name];
    for (auto &m: overloads) {
        if (m.matchSignature(args))
            return &m;
    }

    throw std::runtime_error("Нет подходящей перегрузки метода '" + cn + "." + name + "'");
}


bool Semanter::typesCompatible(const TypeInfo &dest, const TypeInfo &src) const {
    if (dest.isArray || src.isArray)
        return (dest == src);

    if (dest.isClass() || src.isClass())
        return (dest == src);

    if (dest == src) return true;

    if (src.baseType == Token::Type::KwChar && dest.baseType == Token::Type::KwInt)
        return true;
    if (src.baseType == Token::Type::KwBool && dest.baseType == Token::Type::KwInt)
        return true;
    if (src.baseType == Token::Type::KwInt && dest.baseType == Token::Type::KwFloat)
        return true;

    return false;
}

TypeInfo Semanter::commonNumericType(const TypeInfo &a, const TypeInfo &b) const {
    if (a.isClass() || b.isClass())
        throw std::runtime_error("Операции над объектами классов запрещены");
    if (a.isArray || b.isArray)
        throw std::runtime_error("Операции над массивами запрещены");

    if (!a.isNumeric() || !b.isNumeric())
        throw std::runtime_error("Операция над нечисловыми типами");

    if (a.baseType == Token::Type::KwFloat || b.baseType == Token::Type::KwFloat)
        return TypeInfo(Token::Type::KwFloat);

    if (a.baseType == Token::Type::KwInt || b.baseType == Token::Type::KwInt)
        return TypeInfo(Token::Type::KwInt);

    if (a.baseType == Token::Type::KwChar && b.baseType == Token::Type::KwChar)
        return TypeInfo(Token::Type::KwChar);

    if (a.baseType == Token::Type::KwBool || b.baseType == Token::Type::KwBool)
        return TypeInfo(Token::Type::KwInt);

    throw std::runtime_error("Недопустимые типы для числовой операции");
}


bool Semanter::checkUnaryOp(Token::Type op) {
    if (typeStack.empty())
        throw std::runtime_error("Недостаточно операндов для унарного оператора");

    TypeInfo t = popType();

    switch (op) {
        case Token::Type::PlusPlus:
        case Token::Type::MinusMinus:
        case Token::Type::Minus:
            if (!t.isNumeric())
                throw std::runtime_error("Унарный оператор применён к нечисловому типу");
            pushType(t);
            return true;

        case Token::Type::Tilde:
            if (!t.isIntegral())
                throw std::runtime_error("Оператор ~ применим только к целым типам");
            pushType(t);
            return true;

        case Token::Type::Exclamation:
            if (!t.isBool() && !t.isIntegral())
                throw std::runtime_error("Оператор ! применён к не-логическому типу");
            pushType(TypeInfo(Token::Type::KwBool));
            return true;

        default:
            throw std::runtime_error("Неизвестный унарный оператор");
    }
}


bool Semanter::checkBinaryOp(Token::Type op) {
    if (typeStack.size() < 2)
        throw std::runtime_error("Недостаточно операндов для бинарного оператора");

    TypeInfo b = popType();
    TypeInfo a = popType();

    if ((a.isArray || b.isArray) && (
            op == Token::Type::EqualEqual ||
            op == Token::Type::NotEqual ||
            op == Token::Type::Less ||
            op == Token::Type::Greater ||
            op == Token::Type::LessEqual ||
            op == Token::Type::GreaterEqual)) {
        throw std::runtime_error("Сравнение массивов запрещено");
    }

    if ((a.isClass() || b.isClass()) && (
            op == Token::Type::EqualEqual ||
            op == Token::Type::NotEqual)) {
        if (!(a == b))
            throw std::runtime_error("Сравнение объектов разных классов запрещено");
        pushType(TypeInfo(Token::Type::KwBool));
        return true;
    }

    switch (op) {
        case Token::Type::Plus:
        case Token::Type::Minus:
        case Token::Type::Asterisk:
        case Token::Type::Slash: {
            if (!a.isNumeric() || !b.isNumeric())
                throw std::runtime_error("Арифметика возможна только над числовыми типами");
            TypeInfo res = commonNumericType(a, b);
            pushType(res);
            return true;
        }

        case Token::Type::Percent: {
            if (!a.isIntegral() || !b.isIntegral())
                throw std::runtime_error("Оператор % применим только к целым типам");
            TypeInfo res = commonNumericType(a, b);
            pushType(res);
            return true;
        }

        case Token::Type::Shl:
        case Token::Type::Shr:
        case Token::Type::Caret:
        case Token::Type::Ampersand:
        case Token::Type::VerticalBar: {
            if (!a.isIntegral() || !b.isIntegral())
                throw std::runtime_error("Битовые операции допускаются только для целых типов");
            TypeInfo res = commonNumericType(a, b);
            pushType(res);
            return true;
        }

        case Token::Type::Less:
        case Token::Type::Greater:
        case Token::Type::LessEqual:
        case Token::Type::GreaterEqual:
        case Token::Type::EqualEqual:
        case Token::Type::NotEqual:
            if (!a.isNumeric() || !b.isNumeric())
                throw std::runtime_error("Сравнение возможно только над числовыми типами");
            pushType(TypeInfo(Token::Type::KwBool));
            return true;

        case Token::Type::AmpAmp:
        case Token::Type::PipePipe:
            if (!a.isBool() && !a.isIntegral())
                throw std::runtime_error("Нелогический тип в логической операции");
            if (!b.isBool() && !b.isIntegral())
                throw std::runtime_error("Нелогический тип в логической операции");
            pushType(TypeInfo(Token::Type::KwBool));
            return true;

        default:
            throw std::runtime_error("Неизвестный бинарный оператор");
    }
}


bool Semanter::checkAssignment(const TypeInfo &left, const TypeInfo &right) {
    if (!typesCompatible(left, right)) {
        throw std::runtime_error("Несовместимые типы при присваивании: " +
                                 left.toString() + " = " + right.toString());
    }
    pushType(left);
    return true;
}


bool Semanter::checkIfCondition(const TypeInfo &cond) {
    if (!cond.isBool() && !cond.isIntegral())
        throw std::runtime_error("Условие if должно быть логическим или integral");
    return true;
}

bool Semanter::checkReturn(const TypeInfo &expected, const TypeInfo &actual) {
    if (expected.isVoid()) {
        if (!actual.isVoid())
            throw std::runtime_error("Функция void не может возвращать значение");
        return true;
    }
    if (!typesCompatible(expected, actual))
        throw std::runtime_error("Неверный тип в операторе return");
    return true;
}

bool Semanter::checkPrint(const TypeInfo &t) {
    if (t.isVoid())
        throw std::runtime_error("print не может печатать void");
    return true;
}

bool Semanter::checkRead(const TypeInfo &t) {
    if (t.isVoid())
        throw std::runtime_error("read не может читать void");
    return true;
}


bool Semanter::beginFunctionCall(const std::string &name) {
    callStack.push_back(CallContext{});
    auto &ctx = callStack.back();
    ctx.functionName = name;
    ctx.className = "";
    ctx.isMethod = false;
    return true;
}

bool Semanter::beginMethodCall(const std::string &cn, const std::string &name) {
    callStack.push_back(CallContext{});
    auto &ctx = callStack.back();
    ctx.functionName = name;
    ctx.className = cn;
    ctx.isMethod = true;
    return true;
}

bool Semanter::addCallArg() {
    if (callStack.empty())
        throw std::runtime_error("Нет активного вызова");
    if (typeStack.empty())
        throw std::runtime_error("Нет аргумента у вызова");
    callStack.back().argTypes.push_back(popType());
    return true;
}

bool Semanter::endFunctionCall() {
    if (callStack.empty() || callStack.back().isMethod)
        throw std::runtime_error("endFunctionCall без beginFunctionCall");

    CallContext ctx = callStack.back();
    callStack.pop_back();

    FunctionEntry *f = resolveFunction(ctx.functionName, ctx.argTypes);
    pushType(f->returnType);
    return true;
}

bool Semanter::endMethodCall() {
    if (callStack.empty() || !callStack.back().isMethod)
        throw std::runtime_error("endMethodCall без beginMethodCall");

    CallContext ctx = callStack.back();
    callStack.pop_back();

    MethodEntry *m = resolveMethod(ctx.className, ctx.functionName, ctx.argTypes);
    pushType(m->returnType);
    return true;
}


bool Semanter::checkArrayIndex(TypeInfo arrayType,
                               TypeInfo index,
                               std::optional<int> literalIndex) {
    if (!arrayType.isArray)
        throw std::runtime_error("Оператор [] применяется не к массиву");

    if (!index.isIntegral())
        throw std::runtime_error("Индекс массива должен быть integral");

    if (literalIndex.has_value() && arrayType.arraySize != -1) {
        int idx = *literalIndex;
        int size = arrayType.arraySize;
        if (idx < 0 || idx >= size) {
            throw std::runtime_error(
                "Индекс массива выходит за границы: " +
                std::to_string(idx) + " при размере " +
                std::to_string(size)
            );
        }
    }

    pushType(*arrayType.elementType);
    return true;
}

bool Semanter::checkField(const TypeInfo &obj, const std::string &field) {
    if (!obj.isClass())
        throw std::runtime_error("Поле запрашивается не у объекта класса");

    ClassEntry *c = lookupClass(obj.className);
    if (!c)
        throw std::runtime_error("Класс '" + obj.className + "' не найден");

    auto it = c->fields.find(field);
    if (it == c->fields.end())
        throw std::runtime_error("Поле '" + field + "' не существует в классе " + obj.className);

    pushType(it->second.type);
    return true;
}


TypeInfo Semanter::getLiteralType(const Token &tok) const {
    switch (tok.type) {
        case Token::Type::IntegerLiteral:
            return TypeInfo(Token::Type::KwInt);
        case Token::Type::FloatLiteral:
            return TypeInfo(Token::Type::KwFloat);
        case Token::Type::CharLiteral:
            return TypeInfo(Token::Type::KwChar);
        case Token::Type::KwTrue:
        case Token::Type::KwFalse:
            return TypeInfo(Token::Type::KwBool);
        case Token::Type::StringLiteral: {
            TypeInfo elem(Token::Type::KwChar);
            return TypeInfo::makeArray(elem, -1);
        }
        default:
            throw std::runtime_error("Неизвестный литерал");
    }
}

TypeInfo Semanter::makeType(Token::Type t, const std::string &className) {
    if (t == Token::Type::Identifier) {
        TypeInfo res(Token::Type::Identifier);
        res.className = className;
        return res;
    }
    return TypeInfo(t);
}


void Semanter::reset() {
    tidStack.clear();
    scopeLevel = 0;
    classes.clear();
    functions.clear();
    while (!typeStack.empty()) typeStack.pop();
    callStack.clear();
    currentClass.clear();
    enterScope();
}

void Semanter::debug() {
    std::cout << "\n[Semanter] Debug info:\n";
    std::cout << "  Current scope = " << scopeLevel << "\n";
    std::cout << "  Classes   = " << classes.size() << "\n";
    std::cout << "  Functions = " << functions.size() << "\n";
}
