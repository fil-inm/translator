#include "semanter.hpp"

Semanter::Semanter() : currentScopeLevel(0), currentClass("") {
    enterScope(); // Глобальная область видимости
}

// Управление областями видимости
void Semanter::enterScope() {
    currentScopeLevel++;
    tidStack.push_back(std::make_unique<TID>(currentScopeLevel));
}

void Semanter::leaveScope() {
    if (!tidStack.empty()) {
        tidStack.pop_back();
        currentScopeLevel = tidStack.empty() ? 0 : tidStack.back()->getScopeLevel();
    }
}

TID* Semanter::getCurrentTID() {
    return tidStack.empty() ? nullptr : tidStack.back().get();
}

const TID* Semanter::getCurrentTID() const {
    return tidStack.empty() ? nullptr : tidStack.back().get();
}

// Объявление переменных
bool Semanter::declareVariable(const std::string& name, TypeInfo type, bool isConst) {
    if (tidStack.empty()) return false;
    
    if (tidStack.back()->exists(name)) {
        return false; // Дублирование
    }
    
    return tidStack.back()->insert(name, type, isConst);
}

bool Semanter::declareArray(const std::string& name, TypeInfo elementType, int size) {
    if (size <= 0) return false;
    
    TypeInfo arrayType(elementType.baseType, elementType.className, true, size, elementType.baseType);
    return declareVariable(name, arrayType);
}

// Поиск переменной
SymbolEntry* Semanter::lookupVariable(const std::string& name) {
    // Ищем от текущей области к глобальной
    for (auto it = tidStack.rbegin(); it != tidStack.rend(); ++it) {
        SymbolEntry* entry = (*it)->find(name);
        if (entry) return entry;
    }
    
    // Проверяем поля текущего класса
    if (!currentClass.empty()) {
        ClassEntry* classEntry = lookupClass(currentClass);
        if (classEntry && classEntry->hasField(name)) {
            return &classEntry->fields[name];
        }
    }
    
    return nullptr;
}

const SymbolEntry* Semanter::lookupVariable(const std::string& name) const {
    for (auto it = tidStack.rbegin(); it != tidStack.rend(); ++it) {
        const SymbolEntry* entry = (*it)->find(name);
        if (entry) return entry;
    }
    
    if (!currentClass.empty()) {
        auto classIt = classes.find(currentClass);
        if (classIt != classes.end()) {
            auto fieldIt = classIt->second.fields.find(name);
            if (fieldIt != classIt->second.fields.end()) {
                return &fieldIt->second;
            }
        }
    }
    
    return nullptr;
}

// Работа с классами
bool Semanter::declareClass(const std::string& name) {
    if (classes.find(name) != classes.end()) {
        return false;
    }
    
    classes.emplace(name, ClassEntry(name));
    return true;
}

bool Semanter::addClassField(const std::string& className, const std::string& fieldName, TypeInfo type) {
    ClassEntry* classEntry = lookupClass(className);
    if (!classEntry) return false;
    
    if (classEntry->hasField(fieldName)) {
        return false;
    }
    
    classEntry->fields.emplace(fieldName, SymbolEntry(fieldName, type, false, 0));
    return true;
}

bool Semanter::addClassMethod(const std::string& className, const std::string& methodName,
                            const std::string& uniqueName, TypeInfo returnType) {
    ClassEntry* classEntry = lookupClass(className);
    if (!classEntry) return false;
    
    if (classEntry->hasMethod(methodName)) {
        return false;
    }
    
    MethodEntry method(className, methodName, uniqueName, returnType);
    classEntry->methods.emplace(methodName, std::move(method));
    
    methods[className].emplace(methodName, MethodEntry(className, methodName, uniqueName, returnType));
    return true;
}

ClassEntry* Semanter::lookupClass(const std::string& name) {
    auto it = classes.find(name);
    return it != classes.end() ? &it->second : nullptr;
}

const ClassEntry* Semanter::lookupClass(const std::string& name) const {
    auto it = classes.find(name);
    return it != classes.end() ? &it->second : nullptr;
}

void Semanter::setCurrentClass(const std::string& className) {
    currentClass = className;
}

void Semanter::clearCurrentClass() {
    currentClass = "";
}

// Работа с функциями
bool Semanter::declareFunction(const std::string& name, const std::string& uniqueName, TypeInfo returnType) {
    if (functions.find(uniqueName) != functions.end()) {
        return false;
    }
    
    functions.emplace(uniqueName, FunctionEntry(name, uniqueName, returnType));
    return true;
}

bool Semanter::addFunctionParam(const std::string& funcName, const std::string& paramName, TypeInfo type) {
    FunctionEntry* func = lookupFunction(funcName);
    if (!func) return false;
    
    func->params.push_back(ParamInfo(paramName, type));
    return true;
}

FunctionEntry* Semanter::lookupFunction(const std::string& name) {
    auto it = functions.find(name);
    return it != functions.end() ? &it->second : nullptr;
}

const FunctionEntry* Semanter::lookupFunction(const std::string& name) const {
    auto it = functions.find(name);
    return it != functions.end() ? &it->second : nullptr;
}

// Работа с методами
bool Semanter::declareMethod(const std::string& className, const std::string& methodName,
                           const std::string& uniqueName, TypeInfo returnType) {
    if (methods.find(className) == methods.end()) {
        methods[className] = std::unordered_map<std::string, MethodEntry>();
    }
    
    if (methods[className].find(uniqueName) != methods[className].end()) {
        return false;
    }
    
    methods[className].emplace(uniqueName, MethodEntry(className, methodName, uniqueName, returnType));
    return true;
}

bool Semanter::addMethodParam(const std::string& className, const std::string& methodName,
                            const std::string& paramName, TypeInfo type) {
    MethodEntry* method = lookupMethod(className, methodName);
    if (!method) return false;
    
    method->params.push_back(ParamInfo(paramName, type));
    return true;
}

MethodEntry* Semanter::lookupMethod(const std::string& className, const std::string& methodName) {
    auto classIt = methods.find(className);
    if (classIt == methods.end()) return nullptr;
    
    auto methodIt = classIt->second.find(methodName);
    return methodIt != classIt->second.end() ? &methodIt->second : nullptr;
}

const MethodEntry* Semanter::lookupMethod(const std::string& className, const std::string& methodName) const {
    auto classIt = methods.find(className);
    if (classIt == methods.end()) return nullptr;
    
    auto methodIt = classIt->second.find(methodName);
    return methodIt != classIt->second.end() ? &methodIt->second : nullptr;
}

// Работа с конструкторами
bool Semanter::addConstructor(const std::string& className, const std::string& uniqueName) {
    ClassEntry* classEntry = lookupClass(className);
    if (!classEntry) return false;
    
    classEntry->constructors.push_back(uniqueName);
    return true;
}

// Стек типов
void Semanter::pushType(TypeInfo type) {
    typeStack.push(type);
}

void Semanter::pushType(Token::Type type) {
    typeStack.push(TypeInfo(type));
}

TypeInfo Semanter::popType() {
    if (typeStack.empty()) {
        return TypeInfo(Token::Type::EndOfFile);
    }
    
    TypeInfo top = typeStack.top();
    typeStack.pop();
    return top;
}

TypeInfo Semanter::peekType() const {
    if (typeStack.empty()) {
        return TypeInfo(Token::Type::EndOfFile);
    }
    return typeStack.top();
}

bool Semanter::isTypeStackEmpty() const {
    return typeStack.empty();
}

void Semanter::clearTypeStack() {
    while (!typeStack.empty()) {
        typeStack.pop();
    }
}

// Проверка совместимости типов
bool Semanter::typesCompatible(TypeInfo t1, TypeInfo t2, bool forAssignment) const {
    if (t1 == t2) return true;
    
    // Неявное преобразование числовых типов
    if (t1.isNumeric() && t2.isNumeric()) {
        return true;
    }
    
    // Для присваивания более строгие правила
    if (forAssignment) {
        // Можно присваивать int в float, char в int, но не наоборот
        if (t1.baseType == Token::Type::KwFloat && t2.baseType == Token::Type::KwInt) {
            return true;
        }
        if (t1.baseType == Token::Type::KwInt && t2.baseType == Token::Type::KwChar) {
            return true;
        }
    }
    
    return false;
}

// Получение общего типа для бинарных операций
TypeInfo Semanter::getCommonType(TypeInfo t1, TypeInfo t2, Token::Type op) const {
    // Для операций сравнения и логических операций - bool
    if (op == Token::Type::Less || op == Token::Type::Greater ||
        op == Token::Type::LessEqual || op == Token::Type::GreaterEqual ||
        op == Token::Type::EqualEqual || op == Token::Type::NotEqual ||
        op == Token::Type::AmpAmp || op == Token::Type::PipePipe) {
        return TypeInfo(Token::Type::KwBool);
    }
    
    // Для арифметических операций
    if (t1.baseType == Token::Type::KwFloat || t2.baseType == Token::Type::KwFloat) {
        return TypeInfo(Token::Type::KwFloat);
    }
    
    if (t1.baseType == Token::Type::KwInt || t2.baseType == Token::Type::KwInt) {
        return TypeInfo(Token::Type::KwInt);
    }
    
    if (t1.baseType == Token::Type::KwChar && t2.baseType == Token::Type::KwChar) {
        return TypeInfo(Token::Type::KwChar);
    }
    
    return TypeInfo(Token::Type::EndOfFile); // Ошибка
}

// Проверка бинарных операций
bool Semanter::checkBinaryOp(Token::Type op) {
    if (typeStack.size() < 2) {
        return false;
    }
    
    TypeInfo right = popType();
    TypeInfo left = popType();
    
    bool isValid = false;
    TypeInfo resultType(Token::Type::EndOfFile);
    
    switch(op) {
        // Арифметические операции
        case Token::Type::Plus:
        case Token::Type::Minus:
        case Token::Type::Asterisk:
        case Token::Type::Slash:
        case Token::Type::Percent:
            if (left.isNumeric() && right.isNumeric()) {
                isValid = true;
                resultType = getCommonType(left, right, op);
            }
            break;
            
        // Операции сравнения
        case Token::Type::Less:
        case Token::Type::Greater:
        case Token::Type::LessEqual:
        case Token::Type::GreaterEqual:
            if (left.isNumeric() && right.isNumeric()) {
                isValid = true;
                resultType = TypeInfo(Token::Type::KwBool);
            }
            break;
            
        // Операции равенства
        case Token::Type::EqualEqual:
        case Token::Type::NotEqual:
            if (typesCompatible(left, right) || (left.isNumeric() && right.isNumeric())) {
                isValid = true;
                resultType = TypeInfo(Token::Type::KwBool);
            }
            break;
            
        // Битовые операции
        case Token::Type::Ampersand:
        case Token::Type::VerticalBar:
        case Token::Type::Caret:
        case Token::Type::Shl:
        case Token::Type::Shr:
            if (left.isIntegral() && right.isIntegral()) {
                isValid = true;
                resultType = getCommonType(left, right, op);
            }
            break;
            
        // Логические операции
        case Token::Type::AmpAmp:
        case Token::Type::PipePipe:
            if (left.isBool() && right.isBool()) {
                isValid = true;
                resultType = TypeInfo(Token::Type::KwBool);
            }
            break;
            
        default:
            break;
    }
    
    if (isValid) {
        pushType(resultType);
    } else {
        // Возвращаем типы обратно
        pushType(left);
        pushType(right);
    }
    
    return isValid;
}

// Проверка унарных операций
bool Semanter::checkUnaryOp(Token::Type op) {
    if (typeStack.empty()) {
        return false;
    }
    
    TypeInfo operand = popType();
    bool isValid = false;
    TypeInfo resultType(Token::Type::EndOfFile);
    
    switch(op) {
        case Token::Type::Minus:
        case Token::Type::PlusPlus:
        case Token::Type::MinusMinus:
            if (operand.isNumeric()) {
                isValid = true;
                resultType = operand;
            }
            break;
            
        case Token::Type::Exclamation:
            if (operand.isBool()) {
                isValid = true;
                resultType = TypeInfo(Token::Type::KwBool);
            }
            break;
            
        case Token::Type::Tilde:
            if (operand.isIntegral()) {
                isValid = true;
                resultType = operand;
            }
            break;
            
        default:
            break;
    }
    
    if (isValid) {
        pushType(resultType);
    } else {
        pushType(operand);
    }
    
    return isValid;
}

// Проверка присваивания
bool Semanter::checkAssignment(const std::string& identifier) {
    if (typeStack.size() < 2) {
        return false;
    }
    
    TypeInfo right = popType();
    TypeInfo left = popType();
    
    bool isCompatible = typesCompatible(left, right, true);
    
    if (isCompatible) {
        pushType(right); // Результат присваивания - значение справа
    } else {
        pushType(left);
        pushType(right);
    }
    
    return isCompatible;
}

// Проверка условия if/while
bool Semanter::checkIfCondition() {
    if (typeStack.empty()) {
        return false;
    }
    
    TypeInfo cond = popType();
    return cond.isBool();
}

// Проверка return
bool Semanter::checkReturn(TypeInfo expectedReturnType) {
    if (typeStack.empty()) {
        // return без значения
        return expectedReturnType.isVoid();
    }
    
    TypeInfo actual = popType();
    return typesCompatible(expectedReturnType, actual, true);
}

// Проверка print
bool Semanter::checkPrint() {
    if (typeStack.empty()) {
        return false;
    }
    
    TypeInfo toPrint = popType();
    
    // print может выводить любые типы, кроме void
    return !toPrint.isVoid() && toPrint.baseType != Token::Type::EndOfFile;
}

// Проверка read
bool Semanter::checkRead() {
    if (typeStack.empty()) {
        return false;
    }
    
    TypeInfo target = peekType();
    
    // read может читать в переменные любого типа, кроме void
    return !target.isVoid() && target.baseType != Token::Type::EndOfFile;
}

// Проверка вызова функции
bool Semanter::beginFunctionCall(const std::string& funcName) {
    if (currentCall) {
        return false;
    }
    
    currentCall = std::make_unique<CallContext>();
    currentCall->name = funcName;
    currentCall->isMethod = false;
    
    return true;
}

bool Semanter::beginMethodCall(const std::string& className, const std::string& methodName) {
    if (currentCall) {
        return false;
    }
    
    currentCall = std::make_unique<CallContext>();
    currentCall->name = methodName;
    currentCall->className = className;
    currentCall->isMethod = true;
    
    return true;
}

bool Semanter::addCallArg() {
    if (!currentCall) {
        return false;
    }
    
    if (typeStack.empty()) {
        return false;
    }
    
    currentCall->argTypes.push_back(popType());
    return true;
}

bool Semanter::endFunctionCall() {
    if (!currentCall || currentCall->isMethod) {
        return false;
    }
    
    FunctionEntry* func = lookupFunction(currentCall->name);
    if (!func) {
        currentCall.reset();
        return false;
    }
    
    if (!func->matchesParams(currentCall->argTypes)) {
        currentCall.reset();
        return false;
    }
    
    pushType(func->returnType);
    currentCall.reset();
    return true;
}

bool Semanter::endMethodCall() {
    if (!currentCall || !currentCall->isMethod) {
        return false;
    }
    
    MethodEntry* method = lookupMethod(currentCall->className, currentCall->name);
    if (!method) {
        currentCall.reset();
        return false;
    }
    
    if (!method->matchesParams(currentCall->argTypes)) {
        currentCall.reset();
        return false;
    }
    
    pushType(method->returnType);
    currentCall.reset();
    return true;
}

// Проверка доступа к полям
bool Semanter::checkFieldAccess(const std::string& className, const std::string& fieldName) {
    ClassEntry* classEntry = lookupClass(className);
    if (!classEntry) {
        return false;
    }
    
    auto it = classEntry->fields.find(fieldName);
    if (it == classEntry->fields.end()) {
        return false;
    }
    
    pushType(it->second.type);
    return true;
}

// Проверка доступа к массиву
bool Semanter::checkArrayAccess(TypeInfo arrayType) {
    if (!arrayType.isArray) {
        return false;
    }
    
    if (typeStack.empty()) {
        return false;
    }
    
    TypeInfo index = popType();
    if (!index.isIntegral()) {
        pushType(index);
        return false;
    }
    
    // Тип элемента массива
    TypeInfo elementType(arrayType.elementType);
    pushType(elementType);
    return true;
}

// Получение типа литерала
TypeInfo Semanter::getLiteralType(const Token& token) const {
    switch(token.type) {
        case Token::Type::IntegerLiteral:
            return TypeInfo(Token::Type::KwInt);
        case Token::Type::FloatLiteral:
            return TypeInfo(Token::Type::KwFloat);
        case Token::Type::CharLiteral:
            return TypeInfo(Token::Type::KwChar);
        case Token::Type::StringLiteral:
            return TypeInfo(Token::Type::KwChar, "", true, -1, Token::Type::KwChar);
        case Token::Type::KwTrue:
        case Token::Type::KwFalse:
            return TypeInfo(Token::Type::KwBool);
        default:
            return TypeInfo(Token::Type::EndOfFile);
    }
}

// Получение TypeInfo из Token::Type
TypeInfo Semanter::getTypeFromToken(Token::Type tokenType, const std::string& className) const {
    if (tokenType == Token::Type::Identifier) {
        return TypeInfo(tokenType, className);
    }
    return TypeInfo(tokenType);
}

// Сброс состояния
void Semanter::reset() {
    while (!tidStack.empty()) {
        tidStack.pop_back();
    }
    currentScopeLevel = 0;
    currentClass = "";
    classes.clear();
    functions.clear();
    methods.clear();
    clearTypeStack();
    currentCall.reset();
    
    enterScope();
}

// Отладочная информация
void Semanter::printDebugInfo() const {
    std::cout << "=== Semanter Debug Info ===\n";
    std::cout << "Current scope level: " << currentScopeLevel << "\n";
    std::cout << "Current class: " << (currentClass.empty() ? "(none)" : currentClass) << "\n";
    
    std::cout << "\nClasses (" << classes.size() << "):\n";
    for (const auto& [name, classEntry] : classes) {
        std::cout << "  " << name << ": " << classEntry.fields.size() 
                  << " fields, " << classEntry.methods.size() << " methods\n";
    }
    
    std::cout << "\nFunctions (" << functions.size() << "):\n";
    for (const auto& [name, func] : functions) {
        std::cout << "  " << func.name << " -> " << func.returnType.toString() 
                  << " (" << func.params.size() << " params)\n";
    }
    
    std::cout << "\nCurrent TID symbols:\n";
    if (!tidStack.empty()) {
        for (const auto& [name, sym] : tidStack.back()->getSymbols()) {
            std::cout << "  " << name << ": " << sym.type.toString() 
                      << " (scope " << sym.scopeLevel << ")\n";
        }
    }
    
    std::cout << "Type stack size: " << typeStack.size() << "\n";
    if (!typeStack.empty()) {
        std::cout << "Top type: " << typeStack.top().toString() << "\n";
    }
    std::cout << "===========================\n";
}