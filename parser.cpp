#include "parser.hpp"
#include <iostream>

Parser::Parser(Lexer &lexer, Semanter &semanter, Poliz& poliz) : lex(lexer), sem(semanter), poliz(poliz) {
}

bool Parser::match(Token::Type t)  {
    return lex.currentLexeme().type == t;
}

bool Parser::matchType()  {
    static const std::unordered_set<Token::Type> types = {
        Token::Type::KwInt,
        Token::Type::KwFloat,
        Token::Type::KwVoid,
        Token::Type::KwChar,
        Token::Type::KwBool};

    if (lex.currentLexeme().type == Token::Type::Identifier and lex.peekNextLexeme().type == Token::Type::Identifier) {
        return true;
    }
    return types.contains(lex.currentLexeme().type);
}

bool Parser::expect(Token::Type t, const std::string &what)  {
    if (lex.currentLexeme().type == t) {
        lex.nextLexem();
        return true;
    }
    throw std::runtime_error("expected " + what + ", got " + tokenTypeName(lex.currentLexeme().type));
}


bool Parser::parseProgram() {
    try {
        while (match(Token::Type::KwClass))
            if (!parseClass()) return false;

        while (matchType()) {
            if (!parseFunction()) return false;
        }

        if (!parseMain())
            throw std::runtime_error(" main function");

        if (!match(Token::Type::EndOfFile))
            throw std::runtime_error("unexpected tokens after 'main'");

        return true;
    } catch (const std::exception &e) {
        errLine = lex.currentLexeme().pos.line;
        errCol = lex.currentLexeme().pos.column;
        std::cout << "\033[1;31m"
                << "Error at " << errLine << ":" << errCol << "\n"
                << e.what()
                << "\033[0m\n";
        return false;
    }
}


bool Parser::parseClass() {
    expect(Token::Type::KwClass, "'class'");

    Token nameTok = lex.currentLexeme();
    expect(Token::Type::Identifier, "class name");
    currentClassName = nameTok.lexeme;
    inClass = true;

    sem.declareClass(currentClassName);
    sem.setCurrentClass(currentClassName);

    bool ok = parseClassBody();

    sem.clearCurrentClass();
    inClass = false;
    currentClassName.clear();

    return ok;
}

bool Parser::parseClassBody() {
    expect(Token::Type::LBrace, "'{' to begin class body");

    while (matchType()) {
        if (!parseDeclarationStatement()) return false;
    }

    if (!match(Token::Type::KwConstructor)) {
        throw std::runtime_error("expected at least one constructor");
    }

    if (!parseConstructor())
        return false;

    while (match(Token::Type::KwConstructor)) {
        if (!parseConstructor())
            return false;
    }

    while (matchType()) {
        if (!parseFunction()) return false;
    }
    expect(Token::Type::RBrace, "'}' to close class body");

    return true;
}

bool Parser::parseConstructor() {
    expect(Token::Type::KwConstructor, "'constructor'");

    // делаем уникальное имя конструктора на всякий случай
    std::string ctorName = currentClassName + "_ctor";
    sem.addConstructor(currentClassName, ctorName);

    // параметры конструктора в своём scope
    sem.enterScope();
    if (!parseParameters())
        return false;

    if (!parseBlock())
        return false;
    sem.leaveScope();

    return true;
}


bool Parser::parseFunction() {
    TypeInfo retType = parseTypeSemantic();

    Token nameTok = lex.currentLexeme();
    expect(Token::Type::Identifier, "function name");
    std::string fname = nameTok.lexeme;

    currentFunctionName = fname;
    currentReturnType   = retType;
    hasCurrentFunction  = true;
    currentIsMethod     = inClass;

    // регистрируем функцию/метод
    if (inClass) {
        // метод класса
        std::string uniq = currentClassName + "::" + fname;
        sem.addClassMethod(currentClassName, fname, uniq, retType);
    } else {
        // глобальная функция
        std::string uniq = fname;
        sem.declareFunction(fname, uniq, retType);
    }

    // scope для параметров и тела
    sem.enterScope();
    if (!parseParameters()) return false;
    if (!parseBlock())     return false;
    sem.leaveScope();

    hasCurrentFunction  = false;
    currentFunctionName.clear();
    currentIsMethod = false;

    return true;
}

bool Parser::parseParameters() {
    expect(Token::Type::LParen, "'(' in parameter list");

    if (match(Token::Type::RParen)) {
        lex.nextLexem();
        return true;
    }

    auto addParam = [&](const TypeInfo& t, const std::string& pname) {
        // в сигнатуру функции/метода
        if (hasCurrentFunction) {
            if (currentIsMethod) {
                sem.addMethodParam(currentClassName, currentFunctionName, t);
            } else {
                sem.addFunctionParam(currentFunctionName, pname, t);
            }
        }
        // в локальную область
        sem.declareVariable(pname, t);
    };

    // первый параметр
    {
        TypeInfo t = parseTypeSemantic();
        Token nameTok = lex.currentLexeme();
        expect(Token::Type::Identifier, "parameter name");
        std::string pname = nameTok.lexeme;
        addParam(t, pname);
    }

    while (match(Token::Type::Comma)) {
        lex.nextLexem();
        TypeInfo t = parseTypeSemantic();
        Token nameTok = lex.currentLexeme();
        expect(Token::Type::Identifier, "parameter name");
        std::string pname = nameTok.lexeme;
        addParam(t, pname);
    }

    expect(Token::Type::RParen, "')' after parameters");
    return true;
}

bool Parser::parseMain() {
    expect(Token::Type::KwMain, "'main'");

    currentFunctionName = "main";
    currentReturnType   = TypeInfo(Token::Type::KwVoid);
    hasCurrentFunction  = true;
    currentIsMethod     = false;

    sem.declareFunction("main", "main", currentReturnType);

    sem.enterScope();
    if (!parseParameters())
        return false;

    if (!parseBlock())
        return false;
    sem.leaveScope();

    hasCurrentFunction = false;
    currentFunctionName.clear();

    return true;
}


bool Parser::parseType()  {
    switch (lex.currentLexeme().type) {
        case Token::Type::KwInt:
        case Token::Type::KwChar:
        case Token::Type::KwBool:
        case Token::Type::KwFloat:
        case Token::Type::KwVoid:
            lex.nextLexem();
            return true;
        case Token::Type::Identifier:
            if (lex.peekNextLexeme().type == Token::Type::Identifier) {
                lex.nextLexem();
                return true;
            }
        default:
            throw std::runtime_error("expected type");
    }
}

TypeInfo Parser::parseTypeSemantic() {
    Token tok = lex.currentLexeme();

    switch (tok.type) {
        // Базовые типы
        case Token::Type::KwInt:
        case Token::Type::KwChar:
        case Token::Type::KwBool:
        case Token::Type::KwFloat:
        case Token::Type::KwVoid: {
            // Сдвигаем лексер вперёд
            lex.nextLexem();
            // Строим TypeInfo через семантер
            return sem.makeType(tok.type);
        }

            // Имя класса (пользовательский тип)
        case Token::Type::Identifier: {
            // Случай "MyClass x;" — сейчас токен = "MyClass",
            // следующий токен должен быть идентификатором имени переменной.
            if (lex.peekNextLexeme().type == Token::Type::Identifier) {
                std::string className = tok.lexeme;  // <-- если поле называется иначе, поправь здесь
                lex.nextLexem();                     // съедаем идентификатор-типа
                return sem.makeType(Token::Type::Identifier, className);
            }
            // Если следующий токен не Identifier — это не тип, а просто обычный идентификатор
            [[fallthrough]];
        }

        default:
            throw std::runtime_error("expected type");
    }
}

bool Parser::parseLiteral() {
    Token tok = lex.currentLexeme();

    switch (tok.type) {
        case Token::Type::IntegerLiteral:
        case Token::Type::FloatLiteral:
        case Token::Type::CharLiteral:
        case Token::Type::StringLiteral:
        case Token::Type::KwTrue:
        case Token::Type::KwFalse: {
            TypeInfo t = sem.getLiteralType(tok);
            sem.pushType(t);
            lex.nextLexem();
            return true;
        }
        default:
            throw std::runtime_error("expected literal (int, float, char, string, or bool)");
    }
}

bool Parser::parseLValue() {
    Token baseTok = lex.currentLexeme();
    expect(Token::Type::Identifier, "identifier");
    std::string baseName = baseTok.lexeme;

    bool hasTypeOnStack = false;

    auto pushVarType = [&]() {
        if (!hasTypeOnStack) {
            SymbolEntry* sym = sem.lookupVariable(baseName);
            if (!sym)
                throw std::runtime_error("Unknown identifier '" + baseName + "'");
            sem.pushType(sym->type);
            hasTypeOnStack = true;
        }
    };

    while (true) {
        if (match(Token::Type::LBracket)) {
            // массив: a[expr]
            pushVarType();
            lex.nextLexem();

            bool isLiteralIndex = (lex.currentLexeme().type == Token::Type::IntegerLiteral);
            Token indexTok = lex.currentLexeme(); // запомним, если это литерал

            if (!parseExpression())
                return false;

            expect(Token::Type::RBracket, "']' after index");

            TypeInfo index = sem.popType();
            TypeInfo arr   = sem.popType();

            std::optional<int> literalIndex;

            if (isLiteralIndex && indexTok.type == Token::Type::IntegerLiteral) {
                // аккуратно парсим число
                try {
                    literalIndex = std::stoi(indexTok.lexeme);
                } catch (...) {
                    // если вдруг переполнение/мусор — просто не делаем проверку
                }
            }

            sem.checkArrayIndex(arr, index, literalIndex);
            hasTypeOnStack = true;
        }
        else if (match(Token::Type::Dot)) {
            // поле/метод: obj.field или obj.method(...)
            pushVarType();
            lex.nextLexem();
            Token memberTok = lex.currentLexeme();
            expect(Token::Type::Identifier, "member name");
            std::string memberName = memberTok.lexeme;

            if (match(Token::Type::LParen)) {
                // вызов метода obj.method(...)
                TypeInfo objType = sem.popType();
                if (!objType.isClass())
                    throw std::runtime_error("Method call on non-class type");
                std::string cn = objType.className;

                sem.beginMethodCall(cn, memberName);

                expect(Token::Type::LParen, "'(' after method name");
                if (!match(Token::Type::RParen)) {
                    if (!parseExpression())
                        return false;
                    sem.addCallArg();
                    while (match(Token::Type::Comma)) {
                        lex.nextLexem();
                        if (!parseExpression())
                            return false;
                        sem.addCallArg();
                    }
                }
                expect(Token::Type::RParen, "')' after method call");
                sem.endMethodCall();
            } else {
                // просто поле: obj.field
                TypeInfo objType = sem.popType();
                sem.checkField(objType, memberName);
            }
            hasTypeOnStack = true;
        }
        else if (match(Token::Type::LParen)) {
            // вызов функции: f(...)
            if (!hasTypeOnStack) {
                sem.beginFunctionCall(baseName);

                expect(Token::Type::LParen, "'(' after function name");
                if (!match(Token::Type::RParen)) {
                    if (!parseAssignmentExpression())
                        return false;
                    sem.addCallArg();
                    while (match(Token::Type::Comma)) {
                        lex.nextLexem();
                        if (!parseAssignmentExpression())
                            return false;
                        sem.addCallArg();
                    }
                }
                expect(Token::Type::RParen, "')' after function call");
                sem.endFunctionCall();
                hasTypeOnStack = true;
            } else {
                throw std::runtime_error("Calls on non-function values are not supported");
            }
        }
        else {
            break;
        }
    }

    // просто переменная без всего
    if (!hasTypeOnStack) {
        SymbolEntry* sym = sem.lookupVariable(baseName);
        if (!sym)
            throw std::runtime_error("Unknown identifier '" + baseName + "'");
        sem.pushType(sym->type);
    }

    return true;
}

// bool Parser::parseLValueMethod() {
//     expect(Token::Type::Identifier, "lvalue");
//     if (match(Token::Type::LBracket)) {
//         lex.nextLexem();
//         if (!parseLValueMethod())
//             return false;
//         expect(Token::Type::RBracket, "close of array");
//         return true;
//     } else if (match(Token::Type::Dot)) {
//         lex.nextLexem();
//         expect(Token::Type::Identifier, "method name");
//         expect(Token::Type::LParen, "'(' for open method");
//         if (match(Token::Type::RParen)) {
//             lex.nextLexem();
//             return true;
//         } else {
//             if (!parseExpression())
//                 return false;
//             while (match(Token::Type::Comma)) {
//                 lex.nextLexem();
//                 expect(Token::Type::Identifier, "method name");
//             }
//             expect(Token::Type::RParen, "')' after method");
//             return true;
//
//         }
//     } else if (match(Token::Type::LParen)) {
//         lex.nextLexem();
//         if (!parseExpression())
//             return false;
//         while (match(Token::Type::Comma)) {
//             lex.nextLexem();
//             if (!parseExpression())
//                 return false;
//         }
//         expect(Token::Type::RParen, "close of function");
//     }
//     return true;
// }


bool Parser::parseBlock() {
    expect(Token::Type::LBrace, "'{' to begin block");
    sem.enterScope();

    while (!match(Token::Type::RBrace)) {
        parseStatement();
    }

    expect(Token::Type::RBrace, "'}' to close block");
    sem.leaveScope();

    return true;
}

bool Parser::parseStatement() {
    if (matchType()) {
        return parseDeclarationStatement();
    }

    if (match(Token::Type::KwIf))
        return parseIfStatement();

    if (match(Token::Type::KwWhile))
        return parseWhileStatement();

    if (match(Token::Type::KwFor))
        return parseForStatement();

    if (match(Token::Type::KwReturn))
        return parseReturnStatement();

    if (match(Token::Type::KwBreak)) {
        lex.nextLexem();
        expect(Token::Type::Semicolon, "';' after break");
        return true;
    }

    if (match(Token::Type::KwContinue)) {
        lex.nextLexem();
        expect(Token::Type::Semicolon, "';' after continue");
        return true;
    }

    if (match(Token::Type::KwPrint))
        return parsePrintStatement();

    if (match(Token::Type::KwRead))
        return parseReadStatement();

    if (match(Token::Type::LBrace))
        return parseBlock();

    if (parseExpression()) {
        expect(Token::Type::Semicolon, "';' after expression");
        return true;
    }

    throw std::runtime_error("expected statement");
    return false;
}

bool Parser::parseDeclarationStatement() {
    TypeInfo t = parseTypeSemantic();

    Token nameTok = lex.currentLexeme();
    expect(Token::Type::Identifier, "variable or array name");
    std::string name = nameTok.lexeme;

    if (match(Token::Type::LBracket)) {
        expect(Token::Type::LBracket, "[ before size of array");

        Token sizeTok = lex.currentLexeme();
        expect(Token::Type::IntegerLiteral, " integer size of array");
        int size = std::stoi(sizeTok.lexeme);

        expect(Token::Type::RBracket, " ] after size of array");
        expect(Token::Type::Semicolon, " ; after declaration");

        if (inClass) {
            TypeInfo arr = TypeInfo::makeArray(t, size);
            sem.addClassField(currentClassName, name, arr);
        } else {
            sem.declareArray(name, t, size);
        }
        return true;
    }

    expect(Token::Type::Semicolon, " ; after declaration");

    if (inClass) {
        sem.addClassField(currentClassName, name, t);
    } else {
        sem.declareVariable(name, t);
    }

    return true;
}

bool Parser::parseIfStatement() {
    expect(Token::Type::KwIf, "'if'");
    expect(Token::Type::LParen, "'(' after 'if'");

    if (!parseExpression())
        return false;

    TypeInfo cond = sem.popType();
    sem.checkIfCondition(cond);

    expect(Token::Type::RParen, "')' after 'if' condition");

    if (!parseBlock())
        return false;

    if (match(Token::Type::KwElse)) {
        lex.nextLexem();
        if (match(Token::Type::KwIf)) {
            if (!parseIfStatement())
                return false;
        } else {
            if (!parseBlock())
                return false;
        }
    }

    return true;
}

bool Parser::parseWhileStatement() {
    expect(Token::Type::KwWhile, "'while'");
    expect(Token::Type::LParen, "'(' after 'while'");

    if (!parseExpression())
        return false;

    TypeInfo cond = sem.popType();
    sem.checkIfCondition(cond);

    expect(Token::Type::RParen, "')' after 'while' condition");

    if (!parseBlock())
        return false;

    return true;
}

bool Parser::parseForStatement() {
    expect(Token::Type::KwFor, "'for'");
    expect(Token::Type::LParen, "'(' after for");

    // создаём scope для переменной цикла
    sem.enterScope();

    // init: int i = 0
    TypeInfo t = parseTypeSemantic();

    Token nameTok = lex.currentLexeme();
    expect(Token::Type::Identifier, "name of variable");
    std::string name = nameTok.lexeme;
    sem.declareVariable(name, t);

    expect(Token::Type::Assign, "sign =");

    if (!parseExpression())
        return false;
    sem.popType(); // тип инициализации нас не интересует

    expect(Token::Type::Semicolon, "';' after init expression");

    // условие: i < 10
    if (!parseExpression())
        return false;
    TypeInfo cond = sem.popType();
    sem.checkIfCondition(cond); // можно даже проверить, что условие логическое/integral

    expect(Token::Type::Semicolon, "';' after logic expression");

    // шаг: i = i + 1
    if (!parseExpression())
        return false;
    sem.popType(); // тип шага можно игнорировать

    expect(Token::Type::RParen, "')' for close for");

    // И ВОТ ТУТ главное: парсим ТЕЛО цикла как statement
    if (!parseStatement())
        return false;

    sem.leaveScope(); // i живёт только внутри for

    return true;
}

bool Parser::parseReturnStatement() {
    expect(Token::Type::KwReturn, "'return'");

    if (match(Token::Type::Semicolon)) {
        TypeInfo actual(Token::Type::KwVoid);
        if (hasCurrentFunction)
            sem.checkReturn(currentReturnType, actual);
        lex.nextLexem();
        return true;
    }

    if (!parseExpression())
        return false;

    TypeInfo actual = sem.popType();
    if (hasCurrentFunction)
        sem.checkReturn(currentReturnType, actual);

    expect(Token::Type::Semicolon, "';' after return statement");

    return true;
}

bool Parser::parsePrintStatement() {
    expect(Token::Type::KwPrint, "'print'");
    expect(Token::Type::LParen, "'(' after 'print'");

    if (!parseExpression())
        return false;

    TypeInfo t = sem.popType();
    sem.checkPrint(t);

    expect(Token::Type::RParen, "')' after 'print' argument");
    expect(Token::Type::Semicolon, "';' after 'print(...)'");
    return true;
}

bool Parser::parseReadStatement() {
    expect(Token::Type::KwRead, "'read'");
    expect(Token::Type::LParen, "'(' after 'read'");

    if (!parseLValue())
        return false;

    TypeInfo t = sem.popType();
    sem.checkRead(t);

    expect(Token::Type::RParen, "')' after 'read' argument");
    expect(Token::Type::Semicolon, "';' after 'read(...)'");
    return true;
}


bool Parser::parseExpression() {
    if (!parseCommaExpression())
        return false;
    return true;
}

bool Parser::parseCommaExpression() {
    if (!parseAssignmentExpression())
        return false;

    while (match(Token::Type::Comma)) {
        lex.nextLexem();
        if (!parseAssignmentExpression())
            return false;
    }
    return true;
}

// bool Parser::parseApostropheExpression() {
//     if (!parseAssignmentExpression())
//         return false;
//
//     while (lex.currentLexeme().type == Token::Type::Backtick) {
//         lex.nextLexeme();
//         if (!parseAssignmentExpression())
//             return error("expected expression after '`'");
//     }
//     return true;
// }

bool Parser::parseAssignmentExpression() {
    if (!parseLogicalOrExpression())
        return false;

    if (match(Token::Type::Assign)) {
        TypeInfo left = sem.popType();

        expect(Token::Type::Assign, "= in assignment");

        if (!parseAssignmentExpression())
            return false;

        TypeInfo right = sem.popType();
        sem.checkAssignment(left, right);
    }

    return true;
}

bool Parser::parseLogicalOrExpression() {
    if (!parseLogicalAndExpression())
        return false;

    while (match(Token::Type::PipePipe)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseLogicalAndExpression())
            return false;
        sem.checkBinaryOp(op.type);
    }
    return true;
}

bool Parser::parseLogicalAndExpression() {
    if (!parseBitwiseOrExpression())
        return false;

    while (match(Token::Type::AmpAmp)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseBitwiseOrExpression())
            return false;
        sem.checkBinaryOp(op.type);
    }
    return true;
}

bool Parser::parseBitwiseOrExpression() {
    if (!parseBitwiseXorExpression())
        return false;

    while (match(Token::Type::VerticalBar)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseBitwiseXorExpression())
            return false;
        sem.checkBinaryOp(op.type);
    }
    return true;
}

bool Parser::parseBitwiseXorExpression() {
    if (!parseBitwiseAndExpression())
        return false;

    while (match(Token::Type::Caret)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseBitwiseAndExpression())
            return false;
        sem.checkBinaryOp(op.type);
    }
    return true;
}

bool Parser::parseBitwiseAndExpression() {
    if (!parseEqualityExpression())
        return false;

    while (match(Token::Type::Ampersand)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseEqualityExpression())
            return false;
        sem.checkBinaryOp(op.type);
    }
    return true;
}

bool Parser::parseEqualityExpression() {
    if (!parseRelationalExpression())
        return false;

    while (match(Token::Type::EqualEqual) || match(Token::Type::NotEqual)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseRelationalExpression())
            return false;
        sem.checkBinaryOp(op.type);
    }
    return true;
}

bool Parser::parseRelationalExpression() {
    if (!parseShiftExpression())
        return false;

    while (match(Token::Type::Less) ||
           match(Token::Type::Greater) ||
           match(Token::Type::LessEqual) ||
           match(Token::Type::GreaterEqual)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseShiftExpression())
            return false;
        sem.checkBinaryOp(op.type);
           }
    return true;
}

bool Parser::parseShiftExpression() {
    if (!parseAdditiveExpression())
        return false;

    while (match(Token::Type::Shl) || match(Token::Type::Shr)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseAdditiveExpression())
            return false;
        sem.checkBinaryOp(op.type);
    }
    return true;
}

bool Parser::parseAdditiveExpression() {
    if (!parseMultiplicativeExpression())
        return false;

    while (match(Token::Type::Plus) || match(Token::Type::Minus)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseMultiplicativeExpression())
            return false;
        sem.checkBinaryOp(op.type);
    }
    return true;
}

bool Parser::parseMultiplicativeExpression() {
    if (!parseUnaryExpression())
        return false;

    while (match(Token::Type::Asterisk) ||
           match(Token::Type::Slash)    ||
           match(Token::Type::Percent)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseUnaryExpression())
            return false;
        sem.checkBinaryOp(op.type);
           }
    return true;
}

bool Parser::parseUnaryExpression() {
    std::vector<Token::Type> ops;

    while (match(Token::Type::MinusMinus) ||
           match(Token::Type::PlusPlus)   ||
           match(Token::Type::Minus)      ||
           match(Token::Type::Exclamation)||
           match(Token::Type::Tilde)) {
        ops.push_back(lex.currentLexeme().type);
        lex.nextLexem();
           }

    if (!parsePrimaryExpression())
        return false;

    // применяем унарные операторы справа налево
    for (auto it = ops.rbegin(); it != ops.rend(); ++it) {
        sem.checkUnaryOp(*it);
    }

    return true;
}

bool Parser::parsePrimaryExpression() {
    if (match(Token::Type::LParen)) {
        lex.nextLexem();
        if (!parseExpression())
            return false;
        expect(Token::Type::RParen, "')' after expression");
        return true;
    }

    if (match(Token::Type::Identifier))
        return parseLValue();

    return parseLiteral();
}