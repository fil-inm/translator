#include "parser.hpp"
#include <iostream>
#include <optional>

Parser::Parser(Lexer &lexer, Semanter &semanter, Poliz &p)
    : lex(lexer), sem(semanter), poliz(p) {
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

    if (lex.currentLexeme().type == Token::Type::Identifier &&
        lex.peekNextLexeme().type == Token::Type::Identifier) {
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

// ===== Классы / функции / main — как у тебя, без ПОЛИЗа =====
// (ничего не меняем, кроме конструктора Parser)

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

    std::string ctorName = currentClassName + "_ctor";
    sem.addConstructor(currentClassName, ctorName);

sem.enterFunctionScope();
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

    if (inClass) {
        std::string uniq = currentClassName + "::" + fname;
        sem.addClassMethod(currentClassName, fname, uniq, retType);
    } else {
        std::string uniq = fname;
        sem.declareFunction(fname, uniq, retType);
    }

    sem.enterFunctionScope();
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
        if (hasCurrentFunction) {
            if (currentIsMethod) {
                sem.addMethodParam(currentClassName, currentFunctionName, t);
            } else {
                sem.addFunctionParam(currentFunctionName, pname, t);
            }
        }
        sem.declareVariable(pname, t);
    };

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

sem.enterFunctionScope();
    if (!parseParameters())
        return false;

    if (!parseBlock())
        return false;
    sem.leaveScope();

    hasCurrentFunction = false;
    currentFunctionName.clear();

    return true;
}

// ===== Типы =====

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
        case Token::Type::KwInt:
        case Token::Type::KwChar:
        case Token::Type::KwBool:
        case Token::Type::KwFloat:
        case Token::Type::KwVoid: {
            lex.nextLexem();
            return sem.makeType(tok.type);
        }

        case Token::Type::Identifier: {
            if (lex.peekNextLexeme().type == Token::Type::Identifier) {
                std::string className = tok.lexeme;
                lex.nextLexem();
                return sem.makeType(Token::Type::Identifier, className);
            }
            [[fallthrough]];
        }

        default:
            throw std::runtime_error("expected type");
    }
}

// ===== Литералы (ТУТ ПОДКЛЮЧАЕМ POLIZ) =====

bool Parser::parseLiteral() {
    Token tok = lex.currentLexeme();

    switch (tok.type) {
        case Token::Type::IntegerLiteral: {
            TypeInfo t = sem.getLiteralType(tok);
            sem.pushType(t);

            int value = std::stoi(tok.lexeme);
            poliz.emit(Poliz::Op::PUSH_INT, value);

            lex.nextLexem();
            return true;
        }

        case Token::Type::FloatLiteral: {
            TypeInfo t = sem.getLiteralType(tok);
            sem.pushType(t);

            float value = std::stof(tok.lexeme);

            int bits;
            static_assert(sizeof(float) == sizeof(int),
                          "float and int must be same size");
            std::memcpy(&bits, &value, sizeof(float));

            poliz.emit(Poliz::Op::PUSH_FLOAT, bits);

            lex.nextLexem();
            return true;
        }

        case Token::Type::CharLiteral: {
            TypeInfo t = sem.getLiteralType(tok);
            sem.pushType(t);

            char ch = tok.lexeme.size() >= 3 ? tok.lexeme[1] : 0;
            poliz.emit(Poliz::Op::PUSH_CHAR, static_cast<int>(ch));

            lex.nextLexem();
            return true;
        }

        case Token::Type::StringLiteral: {
            TypeInfo t = sem.getLiteralType(tok);
            sem.pushType(t);

            std::string s = tok.lexeme;
            if (!s.empty() && s.front() == '"') s.erase(s.begin());
            if (!s.empty() && s.back()  == '"') s.pop_back();

            int idx = poliz.addString(s);
            poliz.emit(Poliz::Op::PUSH_STRING, idx);

            lex.nextLexem();
            return true;
        }

        case Token::Type::KwTrue:
        case Token::Type::KwFalse: {
            TypeInfo t = sem.getLiteralType(tok);
            sem.pushType(t);

            int v = (tok.type == Token::Type::KwTrue ? 1 : 0);
            poliz.emit(Poliz::Op::PUSH_BOOL, v);

            lex.nextLexem();
            return true;
        }

        default:
            throw std::runtime_error("expected literal (int, float, char, string, or bool)");
    }
}

// ===== LValue (пока без POLIZ для переменных — это следующий этап) =====
// Твой текущий код я оставлю как есть, только не буду пытаться генерить LOAD_VAR/STORE_VAR,
// чтобы не ломать семантику. Для первой проверки достаточно литералов и выражений.
// Если хочешь, можем дальше сюда добавить poliz.emit(LOAD_VAR / STORE_VAR).

bool Parser::parseLValue() {
    Token baseTok = lex.currentLexeme();
    expect(Token::Type::Identifier, "identifier");
    std::string baseName = baseTok.lexeme;

    SymbolEntry* baseSym = sem.lookupVariable(baseName);
    if (!baseSym)
        throw std::runtime_error("Unknown identifier '" + baseName + "'");

    // Изначально это просто переменная
    LValueDesc lval;
    lval.kind = LValueDesc::Kind::Var;
    lval.base = baseSym;

    TypeInfo currentType = baseSym->type;

    while (true) {
        // ---------- a[i] ----------
        if (match(Token::Type::LBracket)) {
            lex.nextLexem();

            if (!parseExpression())
                return false;

            TypeInfo index = sem.popType();
            sem.checkArrayIndex(currentType, index, std::nullopt);

            expect(Token::Type::RBracket, "']'");

            // теперь lvalue — элемент массива
            lval.kind = LValueDesc::Kind::ArrayElem;
            currentType = *currentType.elementType;
        }

        // ---------- a.b ----------
        else if (match(Token::Type::Dot)) {
            lex.nextLexem();
            Token memberTok = lex.currentLexeme();
            expect(Token::Type::Identifier, "member name");

            sem.checkField(currentType, memberTok.lexeme);

            lval.kind  = LValueDesc::Kind::Field;
            lval.field = memberTok.lexeme;
            currentType = sem.peekType();
        }

        else {
            break;
        }
    }

    // 🔥 СОХРАНЯЕМ LVALUE
    lastLValue = lval;

    // 🔥 КЛАДЁМ ТОЛЬКО ТИП
    sem.pushType(currentType);

    return true;
}

// ===== Block / Statements — без POLIZ (кроме print), пока только семантика =====
// Можно потом добавить JUMP/JUMP_IF_FALSE и т.п. Сейчас они не нужны для проверки.

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
        int j = poliz.emitJump(Poliz::Op::JUMP);
        loopStack.back().breaks.push_back(j);
        expect(Token::Type::Semicolon, "';'");
        return true;
    }

    if (match(Token::Type::KwContinue)) {
        lex.nextLexem();
        int j = poliz.emitJump(Poliz::Op::JUMP);
        loopStack.back().continues.push_back(j);
        expect(Token::Type::Semicolon, "';'");
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

// ===== if/while/for/return — пока без ПОЛИЗа (для проверки не надо) =====

bool Parser::parseIfStatement() {
    expect(Token::Type::KwIf, "'if'");
    expect(Token::Type::LParen, "'('");

    parseExpression();
    TypeInfo cond = sem.popType();
    sem.checkIfCondition(cond);

    expect(Token::Type::RParen, "')'");

    int jf = poliz.emitJump(Poliz::Op::JUMP_IF_FALSE);

    parseBlock();

    int jend = -1;

    if (match(Token::Type::KwElse)) {
        jend = poliz.emitJump(Poliz::Op::JUMP);
        poliz.patchJump(jf, poliz.currentIp());

        lex.nextLexem();
        if (match(Token::Type::KwIf))
            parseIfStatement();
        else
            parseBlock();

        poliz.patchJump(jend, poliz.currentIp());
    } else {
        poliz.patchJump(jf, poliz.currentIp());
    }

    return true;
}

bool Parser::parseWhileStatement() {
    expect(Token::Type::KwWhile, "'while'");
    expect(Token::Type::LParen, "'('");

    int start = poliz.currentIp();

    parseExpression();
    TypeInfo cond = sem.popType();
    sem.checkIfCondition(cond);

    expect(Token::Type::RParen, "')'");

    int jf = poliz.emitJump(Poliz::Op::JUMP_IF_FALSE);

    loopStack.push_back({start});

    parseBlock();

    poliz.emit(Poliz::Op::JUMP, start);
    poliz.patchJump(jf, poliz.currentIp());

    for (int b : loopStack.back().breaks)
        poliz.patchJump(b, poliz.currentIp());

    for (int c : loopStack.back().continues)
        poliz.patchJump(c, start);

    loopStack.pop_back();
    return true;
}

bool Parser::parseForStatement() {
    expect(Token::Type::KwFor, "'for'");
    expect(Token::Type::LParen, "'('");


    // ---------- init ----------
    if (!match(Token::Type::Semicolon)) {
        if (!parseExpression()) return false;
        sem.popType(); // результат init не нужен
    }
    expect(Token::Type::Semicolon, "';' after init");

    // ---------- condition ----------
    int condPos = poliz.currentIp();

    if (!match(Token::Type::Semicolon)) {
        if (!parseExpression()) return false;
        TypeInfo cond = sem.popType();
        sem.checkIfCondition(cond);
    } else {
        // пустое условие = true
        poliz.emit(Poliz::Op::PUSH_BOOL, 1);
    }

    int jf = poliz.emitJump(Poliz::Op::JUMP_IF_FALSE);
    expect(Token::Type::Semicolon, "';' after condition");

    // ---------- jump over iter to body (важный хак) ----------
    int jToBody = poliz.emitJump(Poliz::Op::JUMP);

    // ---------- iteration code position ----------
    int iterPos = poliz.currentIp();

    if (!match(Token::Type::RParen)) {
        if (!parseExpression()) return false;
        sem.popType(); // результат iter не нужен
    }

    expect(Token::Type::RParen, "')' after for");

    // после iter надо снова проверить условие
    poliz.emit(Poliz::Op::JUMP, condPos);

    // ---------- body ----------
    int bodyPos = poliz.currentIp();
    poliz.patchJump(jToBody, bodyPos);

    loopStack.push_back({});
    // continue должен идти в iter


    if (!parseStatement()) return false;

    // после тела -> iter
    poliz.emit(Poliz::Op::JUMP, iterPos);

    // ---------- end ----------
    int endPos = poliz.currentIp();
    poliz.patchJump(jf, endPos);

    // break -> end
    for (int b : loopStack.back().breaks)
        poliz.patchJump(b, endPos);

    // continue -> iter
    for (int c : loopStack.back().continues)
        poliz.patchJump(c, iterPos);

    loopStack.pop_back();
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

// ===== print / read — ТУТ ЭМИТИМ PRINT =====

bool Parser::parsePrintStatement() {
    expect(Token::Type::KwPrint, "'print'");
    expect(Token::Type::LParen, "'(' after 'print'");

    if (!parseExpression())
        return false;

    TypeInfo t = sem.popType();
    sem.checkPrint(t);

    // POLIZ: аргумент уже на стеке
    poliz.emit(Poliz::Op::PRINT);

    expect(Token::Type::RParen, "')' after 'print' argument");
    expect(Token::Type::Semicolon, "';' after 'print(...)'");
    return true;
}

bool Parser::parseReadStatement() {
    expect(Token::Type::KwRead, "'read'");
    expect(Token::Type::LParen, "'('");

    if (!parseLValue())
        return false;

    TypeInfo t = sem.popType();
    sem.checkRead(t);

    switch (t.baseType) {
        case Token::Type::KwInt:
            poliz.emit(Poliz::Op::READ_INT);
            break;
        case Token::Type::KwFloat:
            poliz.emit(Poliz::Op::READ_FLOAT);
            break;
        case Token::Type::KwBool:
            poliz.emit(Poliz::Op::READ_BOOL);
            break;
        case Token::Type::KwChar:
            poliz.emit(Poliz::Op::READ_CHAR);
            break;
        default:
            poliz.emit(Poliz::Op::READ_STRING);
            break;
    }

    emitStoreToLValue(*lastLValue);
    lastLValue.reset();

    expect(Token::Type::RParen, "')'");
    expect(Token::Type::Semicolon, "';'");

    return true;
}

// ===== Выражения =====

bool Parser::parseExpression() {
    return parseCommaExpression();
}

bool Parser::parseCommaExpression() {
    if (!parseAssignmentExpression())
        return false;

    while (match(Token::Type::Comma)) {
        lex.nextLexem();
        if (!parseAssignmentExpression())
            return false;
        // семантика запятых пока неважна, можно не эмитить ничего
    }
    return true;
}

bool Parser::parseAssignmentExpression() {
    if (!parseLogicalOrExpression())
        return false;

    if (match(Token::Type::Assign)) {
        if (!lastLValue)
            throw std::runtime_error("Left side is not assignable");

        LValueDesc target = *lastLValue;
        lastLValue.reset();

        TypeInfo left = sem.popType();
        expect(Token::Type::Assign, "=");

        if (!parseAssignmentExpression())
            return false;

        TypeInfo right = sem.popType();
        sem.checkAssignment(left, right);

        emitStoreToLValue(target);
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
        poliz.emit(Poliz::Op::LOG_OR);
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
        poliz.emit(Poliz::Op::LOG_AND);
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
        poliz.emit(Poliz::Op::OR); // если нет — добавь
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
        poliz.emit(Poliz::Op::XOR);
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
        poliz.emit(Poliz::Op::AND);
    }
    return true;
}

// ===== СРАВНЕНИЯ (пока только семантика, можно потом добавить CMP_*) =====

bool Parser::parseEqualityExpression() {
    if (!parseRelationalExpression())
        return false;

    while (match(Token::Type::EqualEqual) || match(Token::Type::NotEqual)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();

        if (!parseRelationalExpression())
            return false;

        sem.checkBinaryOp(op.type);

        poliz.emit(op.type == Token::Type::EqualEqual
                   ? Poliz::Op::CMP_EQ
                   : Poliz::Op::CMP_NE);
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

        switch (op.type) {
            case Token::Type::Less:         poliz.emit(Poliz::Op::CMP_LT); break;
            case Token::Type::Greater:      poliz.emit(Poliz::Op::CMP_GT); break;
            case Token::Type::LessEqual:    poliz.emit(Poliz::Op::CMP_LE); break;
            case Token::Type::GreaterEqual: poliz.emit(Poliz::Op::CMP_GE); break;
            default: break;
        }
           }
    return true;
}

// ===== Сдвиги / арифметика =====

bool Parser::parseShiftExpression() {
    if (!parseAdditiveExpression())
        return false;

    while (match(Token::Type::Shl) || match(Token::Type::Shr)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();

        if (!parseAdditiveExpression())
            return false;

        sem.checkBinaryOp(op.type);
        poliz.emit(op.type == Token::Type::Shl
                   ? Poliz::Op::SHL
                   : Poliz::Op::SHR);
    }
    return true;
}

// ТУТ ТВОЙ ПЕРВЫЙ ПОЛНОЦЕННЫЙ ПОЛИЗ ДЛЯ АРИФМЕТИКИ

bool Parser::parseAdditiveExpression() {
    if (!parseMultiplicativeExpression())
        return false;

    while (match(Token::Type::Plus) || match(Token::Type::Minus)) {
        Token op = lex.currentLexeme();
        lex.nextLexem();
        if (!parseMultiplicativeExpression())
            return false;

        sem.checkBinaryOp(op.type);

        switch (op.type) {
            case Token::Type::Plus:
                poliz.emit(Poliz::Op::ADD);
                break;
            case Token::Type::Minus:
                poliz.emit(Poliz::Op::SUB);
                break;
            default:
                break;
        }
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

        switch (op.type) {
            case Token::Type::Asterisk:
                poliz.emit(Poliz::Op::MUL);
                break;
            case Token::Type::Slash:
                poliz.emit(Poliz::Op::DIV);
                break;
            case Token::Type::Percent:
                poliz.emit(Poliz::Op::MOD);
                break;
            default:
                break;
        }
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

    for (auto it = ops.rbegin(); it != ops.rend(); ++it) {
        sem.checkUnaryOp(*it);

        switch (*it) {
            case Token::Type::Minus:
                poliz.emit(Poliz::Op::NEG);
                break;
            case Token::Type::Exclamation:
                poliz.emit(Poliz::Op::NOT);
                break;
            case Token::Type::Tilde:
                poliz.emit(Poliz::Op::BNOT);
                break;
            case Token::Type::PlusPlus:
            case Token::Type::MinusMinus:
                // пока не реализуем ++/-- в ПОЛИЗ
                break;
            default:
                break;
        }
    }

    return true;
}

bool Parser::parsePrimaryExpression() {
    if (match(Token::Type::LParen)) {
        lex.nextLexem();
        parseExpression();
        expect(Token::Type::RParen, ")");
        return true;
    }

    if (match(Token::Type::Identifier)) {
        parseLValue();

        if (!match(Token::Type::Assign)) {
            emitLoadFromLValue(*lastLValue);
            lastLValue.reset();
        }
        return true;
    }

    return parseLiteral();
}

void Parser::emitLoadFromLValue(const LValueDesc& lv) {
    switch (lv.kind) {
        case LValueDesc::Kind::Var:
            poliz.emit(Poliz::Op::LOAD_VAR, lv.base->slot);
            break;

        case LValueDesc::Kind::ArrayElem:
            // позже: LOAD_ELEM
            throw std::runtime_error("Array load not implemented yet");

        case LValueDesc::Kind::Field:
            // позже: LOAD_FIELD
            throw std::runtime_error("Field load not implemented yet");
    }
}

void Parser::emitStoreToLValue(const LValueDesc& lv) {
    switch (lv.kind) {
        case LValueDesc::Kind::Var:
            poliz.emit(Poliz::Op::STORE_VAR, lv.base->slot);
            break;

        case LValueDesc::Kind::ArrayElem:
            // STORE_ELEM
            throw std::runtime_error("Array store not implemented yet");

        case LValueDesc::Kind::Field:
            // STORE_FIELD
            throw std::runtime_error("Field store not implemented yet");
    }
}