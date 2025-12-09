#pragma once
#include "tokens.hpp"
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <memory>

struct TypeInfo {
    Token::Type baseType;
    std::string className;
    bool isArray;
    int arraySize;
    Token::Type elementType;
    
    TypeInfo(Token::Type bt = Token::Type::KwVoid, const std::string& cn = "", 
             bool ia = false, int as = -1, Token::Type et = Token::Type::EndOfFile)
        : baseType(bt), className(cn), isArray(ia), arraySize(as), elementType(et) {}
    
    bool isClass() const { 
        return baseType == Token::Type::Identifier && !className.empty(); 
    }
    
    bool isVoid() const { return baseType == Token::Type::KwVoid; }
    bool isNumeric() const {
        return baseType == Token::Type::KwInt || 
               baseType == Token::Type::KwFloat || 
               baseType == Token::Type::KwChar;
    }
    bool isIntegral() const {
        return baseType == Token::Type::KwInt || 
               baseType == Token::Type::KwChar || 
               baseType == Token::Type::KwBool;
    }
    bool isBool() const { return baseType == Token::Type::KwBool; }
    
    bool operator==(const TypeInfo& other) const {
        if (baseType != other.baseType) return false;
        if (isArray != other.isArray) return false;
        if (isArray) {
            return arraySize == other.arraySize && elementType == other.elementType;
        }
        if (isClass()) {
            return className == other.className;
        }
        return true;
    }
    
    std::string toString() const {
        if (isArray) {
            std::string elemStr;
            switch(elementType) {
                case Token::Type::KwInt: elemStr = "int"; break;
                case Token::Type::KwFloat: elemStr = "float"; break;
                case Token::Type::KwChar: elemStr = "char"; break;
                case Token::Type::KwBool: elemStr = "bool"; break;
                default: elemStr = "unknown";
            }
            return elemStr + "[" + std::to_string(arraySize) + "]";
        }
        
        if (isClass()) return "class " + className;
        
        return tokenTypeName(baseType);
    }
};

// Запись в таблице идентификаторов (TID)
struct SymbolEntry {
    std::string name;
    TypeInfo type;
    bool isConst;
    int scopeLevel;
    
    SymbolEntry(const std::string& n, TypeInfo t, bool ic = false, int sl = 0)
        : name(n), type(t), isConst(ic), scopeLevel(sl) {}
};

// Параметр функции
struct ParamInfo {
    std::string name;
    TypeInfo type;
    
    ParamInfo(const std::string& n, TypeInfo t) : name(n), type(t) {}
};

// Запись в таблице функций
struct FunctionEntry {
    std::string name;
    std::string uniqueName;
    TypeInfo returnType;
    std::vector<ParamInfo> params;
    int scopeLevel;
    
    FunctionEntry(const std::string& n, const std::string& un, TypeInfo rt, int sl = 0)
        : name(n), uniqueName(un), returnType(rt), scopeLevel(sl) {}
    
    bool matchesParams(const std::vector<TypeInfo>& argTypes) const {
        if (params.size() != argTypes.size()) return false;
        for (size_t i = 0; i < params.size(); i++) {
            if (!(params[i].type == argTypes[i])) return false;
        }
        return true;
    }
};

// Запись в таблице методов
struct MethodEntry : public FunctionEntry {
    std::string className;
    
    MethodEntry(const std::string& cn, const std::string& n, const std::string& un, TypeInfo rt, int sl = 0)
        : FunctionEntry(n, un, rt, sl), className(cn) {}
    
    std::string getFullName() const {
        return className + "::" + name;
    }
};

// Запись в таблице классов
struct ClassEntry {
    std::string name;
    std::unordered_map<std::string, SymbolEntry> fields;
    std::unordered_map<std::string, MethodEntry> methods;
    std::vector<std::string> constructors;
    
    ClassEntry(const std::string& n) : name(n) {}
    
    bool hasField(const std::string& fieldName) const {
        return fields.find(fieldName) != fields.end();
    }
    
    bool hasMethod(const std::string& methodName) const {
        return methods.find(methodName) != methods.end();
    }
};

// Таблица идентификаторов (TID) для одного scope
class TID {
private:
    std::unordered_map<std::string, SymbolEntry> symbols;
    int scopeLevel;
    
public:
    TID(int level = 0) : scopeLevel(level) {}
    
    bool insert(const std::string& name, const TypeInfo& type, bool isConst = false) {
        if (symbols.find(name) != symbols.end()) {
            return false; // Дублирование
        }
        symbols.emplace(name, SymbolEntry(name, type, isConst, scopeLevel));
        return true;
    }
    
    SymbolEntry* find(const std::string& name) {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return &(it->second);
        }
        return nullptr;
    }
    
    const SymbolEntry* find(const std::string& name) const {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return &(it->second);
        }
        return nullptr;
    }
    
    bool exists(const std::string& name) const {
        return symbols.find(name) != symbols.end();
    }
    
    void clear() {
        symbols.clear();
    }
    
    int getScopeLevel() const { return scopeLevel; }
    
    const std::unordered_map<std::string, SymbolEntry>& getSymbols() const {
        return symbols;
    }
};

// Семантический анализатор
class Semanter {
private:
    // Стек TID'ов
    std::vector<std::unique_ptr<TID>> tidStack;
    int currentScopeLevel;
    
    // Таблицы
    std::unordered_map<std::string, ClassEntry> classes;
    std::unordered_map<std::string, FunctionEntry> functions;
    std::unordered_map<std::string, std::unordered_map<std::string, MethodEntry>> methods;
    
    // Стек типов
    std::stack<TypeInfo> typeStack;
    
    // Текущий контекст
    std::string currentClass;
    
    // Контекст текущего вызова
    struct CallContext {
        std::string name;
        std::vector<TypeInfo> argTypes;
        bool isMethod;
        std::string className;
    };
    std::unique_ptr<CallContext> currentCall;
    
    // Вспомогательные методы
    TypeInfo getCommonType(TypeInfo t1, TypeInfo t2, Token::Type op) const;
    bool typesCompatible(TypeInfo t1, TypeInfo t2, bool forAssignment = false) const;
    
public:
    Semanter();
    
    // Управление областями видимости
    void enterScope();
    void leaveScope();
    TID* getCurrentTID();
    const TID* getCurrentTID() const;
    
    // Работа с TID
    bool declareVariable(const std::string& name, TypeInfo type, bool isConst = false);
    bool declareArray(const std::string& name, TypeInfo elementType, int size);
    SymbolEntry* lookupVariable(const std::string& name);
    const SymbolEntry* lookupVariable(const std::string& name) const;
    
    // Работа с классами
    bool declareClass(const std::string& name);
    bool addClassField(const std::string& className, const std::string& fieldName, TypeInfo type);
    bool addClassMethod(const std::string& className, const std::string& methodName, 
                       const std::string& uniqueName, TypeInfo returnType);
    ClassEntry* lookupClass(const std::string& name);
    const ClassEntry* lookupClass(const std::string& name) const;
    void setCurrentClass(const std::string& className);
    void clearCurrentClass();
    std::string getCurrentClass() const { return currentClass; }
    
    // Работа с функциями
    bool declareFunction(const std::string& name, const std::string& uniqueName, TypeInfo returnType);
    bool addFunctionParam(const std::string& funcName, const std::string& paramName, TypeInfo type);
    FunctionEntry* lookupFunction(const std::string& name);
    const FunctionEntry* lookupFunction(const std::string& name) const;
    
    // Работа с методами
    bool declareMethod(const std::string& className, const std::string& methodName, 
                      const std::string& uniqueName, TypeInfo returnType);
    bool addMethodParam(const std::string& className, const std::string& methodName, 
                       const std::string& paramName, TypeInfo type);
    MethodEntry* lookupMethod(const std::string& className, const std::string& methodName);
    const MethodEntry* lookupMethod(const std::string& className, const std::string& methodName) const;
    
    // Работа с конструкторами
    bool addConstructor(const std::string& className, const std::string& uniqueName);
    
    // Стек типов
    void pushType(TypeInfo type);
    void pushType(Token::Type type); // Удобный метод для встроенных типов
    TypeInfo popType();
    TypeInfo peekType() const;
    bool isTypeStackEmpty() const;
    void clearTypeStack();
    
    // Проверка операций
    bool checkBinaryOp(Token::Type op);
    bool checkUnaryOp(Token::Type op);
    bool checkAssignment(const std::string& identifier = "");
    bool checkIfCondition();
    bool checkReturn(TypeInfo expectedReturnType);
    bool checkPrint();
    bool checkRead();
    
    // Проверка вызова
    bool beginFunctionCall(const std::string& funcName);
    bool beginMethodCall(const std::string& className, const std::string& methodName);
    bool addCallArg();
    bool endFunctionCall();
    bool endMethodCall();
    
    // Проверка доступа
    bool checkFieldAccess(const std::string& className, const std::string& fieldName);
    bool checkArrayAccess(TypeInfo arrayType);
    
    // Проверка типов литералов
    TypeInfo getLiteralType(const Token& token) const;
    TypeInfo getTypeFromToken(Token::Type tokenType, const std::string& className = "") const;
    
    // Утилиты
    void reset();
    void printDebugInfo() const;
};