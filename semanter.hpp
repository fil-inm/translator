#pragma once
#include "tokens.hpp"
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <memory>
#include <iostream>

// ============================================================
// TypeInfo — модель типов
// ============================================================

struct TypeInfo {
    Token::Type baseType;
    std::string className;
    bool isArray;
    int arraySize;
    std::unique_ptr<TypeInfo> elementType;

    TypeInfo(Token::Type bt = Token::Type::KwVoid)
        : baseType(bt), className(""), isArray(false), arraySize(-1), elementType(nullptr) {}

    TypeInfo(const TypeInfo& other)
        : baseType(other.baseType),
          className(other.className),
          isArray(other.isArray),
          arraySize(other.arraySize)
    {
        if (other.elementType)
            elementType = std::make_unique<TypeInfo>(*other.elementType);
        else
            elementType = nullptr;
    }

    TypeInfo& operator=(const TypeInfo& other) {
        if (this == &other) return *this;

        baseType    = other.baseType;
        className   = other.className;
        isArray     = other.isArray;
        arraySize   = other.arraySize;

        if (other.elementType)
            elementType = std::make_unique<TypeInfo>(*other.elementType);
        else
            elementType.reset();

        return *this;
    }

    TypeInfo(TypeInfo&&) noexcept = default;
    TypeInfo& operator=(TypeInfo&&) noexcept = default;

    // Фабрика массива
    static TypeInfo makeArray(const TypeInfo& elem, int size) {
        TypeInfo t;
        t.isArray   = true;
        t.arraySize = size;
        t.baseType  = elem.baseType;
        t.className = elem.className;
        t.elementType = std::make_unique<TypeInfo>(elem);
        return t;
    }

    bool isClass() const {
        return baseType == Token::Type::Identifier && !className.empty();
    }

    bool isNumeric() const {
        return baseType == Token::Type::KwInt  ||
               baseType == Token::Type::KwFloat||
               baseType == Token::Type::KwChar ||
               baseType == Token::Type::KwBool;
    }

    bool isIntegral() const {
        return baseType == Token::Type::KwInt  ||
               baseType == Token::Type::KwChar ||
               baseType == Token::Type::KwBool;
    }

    bool isBool() const  { return baseType == Token::Type::KwBool; }
    bool isVoid() const  { return baseType == Token::Type::KwVoid; }

    bool operator==(const TypeInfo& other) const {
        if (isArray || other.isArray) {
            if (!isArray || !other.isArray) return false;
            if (arraySize != other.arraySize) return false;
            return *elementType == *other.elementType;
        }
        if (isClass() || other.isClass()) {
            return baseType == other.baseType && className == other.className;
        }
        return baseType == other.baseType;
    }

    std::string toString() const {
        if (isArray) {
            return elementType->toString() + "[" +
                   (arraySize == -1 ? "?" : std::to_string(arraySize)) + "]";
        }
        if (isClass()) return "class " + className;
        return tokenTypeName(baseType);
    }
};

// ============================================================
// Символы и таблица идентификаторов
// ============================================================

struct SymbolEntry {
    std::string name;
    TypeInfo    type;
    bool        isConst;
    int         scopeLevel;

    SymbolEntry(const std::string& n, const TypeInfo& t, bool c, int sl)
        : name(n), type(t), isConst(c), scopeLevel(sl) {}
};

class TID {
private:
    std::unordered_map<std::string, SymbolEntry> table;
    int scope;

public:
    TID(int lvl) : scope(lvl) {}

    bool insert(const std::string& name, const TypeInfo& t, bool isConst = false) {
        if (table.contains(name)) return false;
        table.emplace(name, SymbolEntry(name, t, isConst, scope));
        return true;
    }

    SymbolEntry* find(const std::string& name) {
        auto it = table.find(name);
        return (it == table.end() ? nullptr : &it->second);
    }

    const SymbolEntry* find(const std::string& name) const {
        auto it = table.find(name);
        return (it == table.end() ? nullptr : &it->second);
    }
};



struct ParamInfo {
    std::string name;
    TypeInfo    type;
};

struct FunctionEntry {
    std::string name;       // логическое имя (f)
    std::string uniqueName; // уникальное для back-end’а (f_int_float и т.п.)
    TypeInfo    returnType;
    std::vector<ParamInfo> params;
    int         scopeLevel;

    FunctionEntry(const std::string& n, const std::string& u, const TypeInfo& r)
        : name(n), uniqueName(u), returnType(r), scopeLevel(0) {}

    bool matchSignature(const std::vector<TypeInfo>& args) const {
        if (args.size() != params.size()) return false;
        for (size_t i = 0; i < args.size(); ++i) {
            if (!(params[i].type == args[i])) return false;
        }
        return true;
    }
};

struct MethodEntry : public FunctionEntry {
    std::string className;

    MethodEntry(const std::string& cn,
                const std::string& n,
                const std::string& u,
                const TypeInfo&   rt)
        : FunctionEntry(n, u, rt), className(cn) {}
};

struct ClassEntry {
    std::string name;
    std::unordered_map<std::string, SymbolEntry>         fields;
    std::unordered_map<std::string, std::vector<MethodEntry>> methods;
    std::vector<std::string> constructors;

    ClassEntry(const std::string& n) : name(n) {}
};

// ============================================================
// Semanter
// ============================================================

class Semanter {
private:
    int scopeLevel = 0;
    std::vector<std::unique_ptr<TID>> tidStack;

    std::unordered_map<std::string, ClassEntry> classes;
    std::unordered_map<std::string, std::vector<FunctionEntry>> functions;

    std::stack<TypeInfo> typeStack;

    std::string currentClass;

    struct CallContext {
        std::string functionName;
        std::string className;
        bool        isMethod = false;
        std::vector<TypeInfo> argTypes;
    };

    std::vector<CallContext> callStack;

public:
    Semanter();

    // области видимости
    void enterScope();
    void leaveScope();
    TID* currentTID();

    // переменные
    bool declareVariable(const std::string& name, const TypeInfo& type, bool isConst = false);
    bool declareArray(const std::string& name, const TypeInfo& type, int size);
    SymbolEntry* lookupVariable(const std::string& name);

    // типовой стек
    void     pushType(const TypeInfo& t);
    TypeInfo popType();
    TypeInfo peekType() const;
    bool     typeEmpty() const;

    // классы
    bool        declareClass(const std::string& name);
    bool        addClassField(const std::string& className, const std::string& field, const TypeInfo& type);
    bool        addClassMethod(const std::string& className,
                               const std::string& methodName,
                               const std::string& uniqueName,
                               const TypeInfo&   returnType);
    bool        addConstructor(const std::string& className, const std::string& uniqueName);
    ClassEntry* lookupClass(const std::string& name);
    void        setCurrentClass(const std::string& cn);
    void        clearCurrentClass();

    // функции
    bool          declareFunction(const std::string& name,
                                  const std::string& uniqueName,
                                  const TypeInfo&    returnType);
    bool          addFunctionParam(const std::string& fname,
                                   const std::string& pname,
                                   const TypeInfo&    type);
    FunctionEntry* resolveFunction(const std::string& name,
                                   const std::vector<TypeInfo>& args);

    // методы
    bool         addMethodParam(const std::string& className,
                                const std::string& methodName,
                                const TypeInfo&    type);
    MethodEntry* resolveMethod(const std::string& className,
                               const std::string& name,
                               const std::vector<TypeInfo>& args);

    // операции
    bool     checkUnaryOp(Token::Type op);
    bool     checkBinaryOp(Token::Type op);
    bool     checkAssignment(const TypeInfo& left, const TypeInfo& right);
    bool     checkIfCondition(const TypeInfo& cond);
    bool     checkReturn(const TypeInfo& expected, const TypeInfo& actual);
    bool     checkPrint(const TypeInfo& t);
    bool     checkRead(const TypeInfo& t);
    bool checkArrayIndex(TypeInfo arrayType, TypeInfo index,
                     std::optional<int> literalIndex = std::nullopt);
    bool     checkField(const TypeInfo& objectType, const std::string& field);

    // вызовы
    bool     beginFunctionCall(const std::string& name);
    bool     beginMethodCall(const std::string& className, const std::string& name);
    bool     addCallArg();
    bool     endFunctionCall();
    bool     endMethodCall();

    // литералы и создание типов
    TypeInfo getLiteralType(const Token& tok) const;
    TypeInfo makeType(Token::Type t, const std::string& className = "");

    // совместимость / приведения
    bool     typesCompatible(const TypeInfo& dest, const TypeInfo& src) const;
    TypeInfo commonNumericType(const TypeInfo& a, const TypeInfo& b) const;

    // служебное
    void reset();
    void debug();
};