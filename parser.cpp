#include "parser.hpp"
#include <iostream>
#include <unordered_set>

Parser::Parser(Lexer &l, Semanter &s, Poliz &p)
    : lex(l), sem(s), poliz(p) {
}

bool Parser::match(Token::Type t) {
    return lex.currentLexeme().type == t;
}

bool Parser::matchType() {
    static const std::unordered_set<Token::Type> types{
        Token::Type::KwInt,
        Token::Type::KwFloat,
        Token::Type::KwChar,
        Token::Type::KwBool,
        Token::Type::KwVoid
    };
    return types.contains(lex.currentLexeme().type);
}

void Parser::expect(Token::Type t, const std::string &what) {
    if (!match(t)) {
        throw std::runtime_error(
            "expected " + what +
            ", got " + tokenTypeName(lex.currentLexeme().type)
        );
    }
    lex.nextLexem();
}

bool Parser::parseProgram() {
    int start = poliz.emitJump(Poliz::Op::JUMP);
    try {
        while (match(Token::Type::KwDeclare))
            parseFunctionDeclaration();

        while (matchType())
            parseFunctionDefinition();

        int mainEntry = poliz.currentIp();
        parseMain();
        poliz.patchJump(start, mainEntry);



        expect(Token::Type::EndOfFile, "EOF");
        poliz.emit(Poliz::Op::HALT);
        return true;
    } catch (const std::exception &e) {
        auto pos = lex.currentLexeme().pos;
        std::cerr << "Error at "
                << pos.line << ":" << pos.column
                << "\n" << e.what() << "\n";
        return false;
    }
}

TypeInfo Parser::parseType() {
    Token tok = lex.currentLexeme();
    switch (tok.type) {
        case Token::Type::KwInt:
        case Token::Type::KwFloat:
        case Token::Type::KwChar:
        case Token::Type::KwBool:
        case Token::Type::KwVoid:
            lex.nextLexem();
            return TypeInfo(tok.type);
        default:
            throw std::runtime_error("expected type");
    }
}

void Parser::parseFunctionDeclaration() {
    expect(Token::Type::KwDeclare, "'declare'");

    TypeInfo ret = parseType();

    Token nameTok = lex.currentLexeme();
    if (match(Token::Type::KwMain)) {
        expect(Token::Type::KwMain, "function main");
    } else {
        expect(Token::Type::Identifier, "function name");
    }
    std::string name = nameTok.lexeme;

    expect(Token::Type::LParen, "(");

    std::vector<TypeInfo> params;
    if (!match(Token::Type::RParen)) {
        do {
            params.push_back(parseType());
            if (!match(Token::Type::Comma))
                break;
            lex.nextLexem();
        } while (true);
    }

    expect(Token::Type::RParen, ")");
    expect(Token::Type::Semicolon, ";");

    FunctionSymbol* fn = sem.declareFunction(name, ret, params);

    fn->entryIp = -1;
    fn->polizIndex = poliz.registerFunction(
        name,
        -1,
        params.size()
    );
}

void Parser::parseFunctionDefinition() {
    TypeInfo ret = parseType();

    Token nameTok = lex.currentLexeme();
    expect(Token::Type::Identifier, "function name");
    std::string name = nameTok.lexeme;

    expect(Token::Type::LParen, "(");

    std::vector<TypeInfo> paramTypes;
    std::vector<std::string> paramNames;

    if (!match(Token::Type::RParen)) {
        do {
            TypeInfo t = parseType();

            Token id = lex.currentLexeme();
            expect(Token::Type::Identifier, "parameter name");

            paramTypes.push_back(t);
            paramNames.push_back(id.lexeme);

            if (!match(Token::Type::Comma))
                break;
            lex.nextLexem();
        } while (true);
    }

    expect(Token::Type::RParen, ")");

    FunctionSymbol* fn = sem.defineFunction(name, ret, paramTypes);

    int skipJump = poliz.emitJump(Poliz::Op::JUMP);

    fn->entryIp = poliz.currentIp();

    poliz.setFunctionEntry(fn->polizIndex, fn->entryIp);

    sem.enterFunctionScope(ret);

    for (size_t i = 0; i < paramNames.size(); ++i) {
        sem.declareVariable(paramNames[i], paramTypes[i]);
    }

    parseBlock();

    sem.leaveScope();

    if (ret.isVoid()) {
        poliz.emit(Poliz::Op::RET_VOID);
    }

    poliz.patchJump(skipJump, poliz.currentIp());
}

void Parser::parseMain() {
    expect(Token::Type::KwMain, "'main'");

    FunctionSymbol* fn =
        sem.defineFunction("main", TypeInfo(Token::Type::KwVoid), {});

    fn->entryIp = poliz.currentIp();
    fn->polizIndex = poliz.registerFunction("main", fn->entryIp, 0);

    sem.enterFunctionScope(TypeInfo(Token::Type::KwVoid));
    parseBlock();
    sem.leaveScope();
    poliz.emit(Poliz::Op::RET_VOID);

}

void Parser::parseBlock() {
    expect(Token::Type::LBrace, "'{'");
    sem.enterScope();

    while (!match(Token::Type::RBrace))
        parseStatement();

    expect(Token::Type::RBrace, "'}'");
    sem.leaveScope();
}

void Parser::parseStatement() {
    if (matchType()) {
        parseDeclaration();
        return;
    }

    if (match(Token::Type::KwIf)) {
        parseIf();
        return;
    }

    if (match(Token::Type::KwWhile)) {
        parseWhile();
        return;
    }

    if (match(Token::Type::KwFor)) {
        parseFor();
        return;
    }

    if (match(Token::Type::KwReturn)) {
        parseReturn();
        return;
    }

    if (match(Token::Type::KwRead)) {
        parseRead();
        return;
    }

    if (match(Token::Type::KwPrint)) {
        parsePrint();
        return;
    }

    if (match(Token::Type::LBrace)) {
        parseBlock();
        return;
    }

    if (match(Token::Type::KwBreak)) {
        if (loopStack.empty())
            throw std::runtime_error("break outside loop");

        lex.nextLexem();
        int j = poliz.emitJump(Poliz::Op::JUMP);
        loopStack.back().breaks.push_back(j);
        expect(Token::Type::Semicolon, ";");
        return;
    }

    if (match(Token::Type::KwContinue)) {
        if (loopStack.empty())
            throw std::runtime_error("continue outside loop");

        lex.nextLexem();
        int j = poliz.emitJump(Poliz::Op::JUMP);
        loopStack.back().continues.push_back(j);
        expect(Token::Type::Semicolon, ";");
        return;
    }

    parseExpression();
    expect(Token::Type::Semicolon, "';'");
}

void Parser::parseDeclaration() {
    TypeInfo base = parseType();

    Token id = lex.currentLexeme();
    expect(Token::Type::Identifier, "variable name");
    std::string name = id.lexeme;

    if (match(Token::Type::LBracket)) {
        lex.nextLexem();
        Token sizeTok = lex.currentLexeme();
        expect(Token::Type::IntegerLiteral, "array size");
        int size = std::stoi(sizeTok.lexeme);
        expect(Token::Type::RBracket, "]");
        expect(Token::Type::Semicolon, ";");

        sem.declareArray(name, base, size);
        return;
    }

    expect(Token::Type::Semicolon, ";");
    sem.declareVariable(name, base);
}

void Parser::parseIf() {
    expect(Token::Type::KwIf, "'if'");
    expect(Token::Type::LParen, "(");

    parseExpression();
    sem.checkIfCondition(sem.popType());

    expect(Token::Type::RParen, ")");

    int jf = poliz.emitJump(Poliz::Op::JUMP_IF_FALSE);
    parseStatement();

    if (match(Token::Type::KwElse)) {
        int jend = poliz.emitJump(Poliz::Op::JUMP);
        poliz.patchJump(jf, poliz.currentIp());
        lex.nextLexem();
        parseStatement();
        poliz.patchJump(jend, poliz.currentIp());
    } else {
        poliz.patchJump(jf, poliz.currentIp());
    }
}

void Parser::parseWhile() {
    expect(Token::Type::KwWhile, "while");
    int start = poliz.currentIp();

    expect(Token::Type::LParen, "(");
    parseExpression();
    sem.checkIfCondition(sem.popType());
    expect(Token::Type::RParen, ")");

    int jf = poliz.emitJump(Poliz::Op::JUMP_IF_FALSE);

    loopStack.push_back({start});

    parseBlock();

    poliz.emit(Poliz::Op::JUMP, start);
    int end = poliz.currentIp();

    poliz.patchJump(jf, end);

    for (int b: loopStack.back().breaks)
        poliz.patchJump(b, end);

    for (int c: loopStack.back().continues)
        poliz.patchJump(c, start);

    loopStack.pop_back();
}

void Parser::parseFor() {
    expect(Token::Type::KwFor, "'for'");
    expect(Token::Type::LParen, "(");

    if (!match(Token::Type::Semicolon)) {
        parseExpression();
        finalizeRValue();

        sem.popType();
    }
    expect(Token::Type::Semicolon, ";");

    int condPos = poliz.currentIp();

    if (!match(Token::Type::Semicolon)) {
        parseExpression();
        finalizeRValue();
        sem.checkIfCondition(sem.popType());
    } else {
        poliz.emit(Poliz::Op::PUSH_BOOL, 1);
    }

    int jf = poliz.emitJump(Poliz::Op::JUMP_IF_FALSE);
    expect(Token::Type::Semicolon, ";");

    int jumpToBody = poliz.emitJump(Poliz::Op::JUMP);

    int iterPos = poliz.currentIp();
    if (!match(Token::Type::RParen)) {
        parseExpression();
        finalizeRValue();

        sem.popType();
    }
    expect(Token::Type::RParen, ")");

    poliz.emit(Poliz::Op::JUMP, condPos);

    int bodyPos = poliz.currentIp();
    poliz.patchJump(jumpToBody, bodyPos);

    loopStack.push_back({iterPos});

    parseStatement();

    poliz.emit(Poliz::Op::JUMP, iterPos);

    int endPos = poliz.currentIp();
    poliz.patchJump(jf, endPos);

    for (int b: loopStack.back().breaks)
        poliz.patchJump(b, endPos);

    for (int c: loopStack.back().continues)
        poliz.patchJump(c, iterPos);

    loopStack.pop_back();
}

void Parser::parseReturn() {
    expect(Token::Type::KwReturn, "'return'");

    if (match(Token::Type::Semicolon)) {
        sem.checkReturn(
            sem.currentReturnType(),
            TypeInfo(Token::Type::KwVoid)
        );
        poliz.emit(Poliz::Op::RET_VOID);
        lex.nextLexem();
        return;
    }

    parseExpression();
    finalizeRValue();

    TypeInfo t = sem.popType();
    sem.checkReturn(sem.currentReturnType(), t);
    poliz.emit(Poliz::Op::RET_VALUE);

    expect(Token::Type::Semicolon, "';'");
}

void Parser::parsePrint() {
    expect(Token::Type::KwPrint, "'print'");
    expect(Token::Type::LParen, "(");

    parseExpression();
    sem.checkPrint(sem.popType());

    poliz.emit(Poliz::Op::PRINT);

    expect(Token::Type::RParen, ")");
    expect(Token::Type::Semicolon, "';'");
}

void Parser::parseRead() {
    expect(Token::Type::KwRead, "'read'");
    expect(Token::Type::LParen, "(");

    parseLValue();

    if (!lastLValue)
        throw std::runtime_error("read() expects variable");

    LValueDesc lv = *lastLValue;
    lastLValue.reset();

    TypeInfo t = sem.popType();

    expect(Token::Type::RParen, ")");
    expect(Token::Type::Semicolon, "';'");

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
            throw std::runtime_error("read(): unsupported type");
    }

    emitStoreToLValue(lv);
}

void Parser::parseExpression() {
    parseComma();
    finalizeRValue();

}

void Parser::parseComma() {
    parseAssignment();
    while (match(Token::Type::Comma)) {
        lex.nextLexem();
        TypeInfo last = sem.popType();
        parseAssignment();
        sem.pushType(last);
    }
}

void Parser::parseAssignment() {
    parseLogicalOr();

    if (match(Token::Type::Assign)) {
        if (!lastLValue)
            throw std::runtime_error("left side is not assignable");

        LValueDesc target = *lastLValue;
        lastLValue.reset();

        TypeInfo left = sem.popType();

        lex.nextLexem();
        parseAssignment();

        finalizeRValue();

        TypeInfo right = sem.popType();
        sem.checkAssignment(left, right);

        emitStoreToLValue(target);
        sem.pushType(left);
    }

}

void Parser::parseLogicalOr() {
    parseLogicalAnd();

    while (match(Token::Type::PipePipe)) {
        finalizeRValue();

        lex.nextLexem();
        parseLogicalAnd();
        sem.checkBinaryOp(Token::Type::PipePipe);
        poliz.emit(Poliz::Op::LOG_OR);

    }
}

void Parser::parseLogicalAnd() {
    parseBitwiseOr();

    while (match(Token::Type::AmpAmp)) {
        finalizeRValue();

        lex.nextLexem();
        parseBitwiseOr();
        sem.checkBinaryOp(Token::Type::AmpAmp);
        poliz.emit(Poliz::Op::LOG_AND);

    }
}

void Parser::parseBitwiseOr() {
    parseBitwiseXor();

    while (match(Token::Type::VerticalBar)) {
        finalizeRValue();

        lex.nextLexem();
        parseBitwiseXor();
        sem.checkBinaryOp(Token::Type::VerticalBar);
        poliz.emit(Poliz::Op::OR);
    }
}

void Parser::parseBitwiseXor() {
    parseBitwiseAnd();

    while (match(Token::Type::Caret)) {
        finalizeRValue();

        lex.nextLexem();
        parseBitwiseAnd();
        sem.checkBinaryOp(Token::Type::Caret);
        poliz.emit(Poliz::Op::XOR);
    }
}

void Parser::parseBitwiseAnd() {
    parseEquality();

    while (match(Token::Type::Ampersand)) {
        finalizeRValue();

        lex.nextLexem();
        parseEquality();
        sem.checkBinaryOp(Token::Type::Ampersand);
        poliz.emit(Poliz::Op::AND);
    }
}

void Parser::parseEquality() {
    parseRelational();

    while (match(Token::Type::EqualEqual) || match(Token::Type::NotEqual)) {
        finalizeRValue();

        Token op = lex.currentLexeme();
        lex.nextLexem();
        parseRelational();
        sem.checkBinaryOp(op.type);
        poliz.emit(
            op.type == Token::Type::EqualEqual
                ? Poliz::Op::CMP_EQ
                : Poliz::Op::CMP_NE
        );
    }
}

void Parser::parseRelational() {
    parseShift();

    while (match(Token::Type::Less) || match(Token::Type::Greater) ||
           match(Token::Type::LessEqual) || match(Token::Type::GreaterEqual)) {

        finalizeRValue();

        Token op = lex.currentLexeme();
        lex.nextLexem();

        parseShift();
        finalizeRValue();

        sem.checkBinaryOp(op.type);

        switch (op.type) {
            case Token::Type::Less:         poliz.emit(Poliz::Op::CMP_LT); break;
            case Token::Type::LessEqual:    poliz.emit(Poliz::Op::CMP_LE); break;
            case Token::Type::Greater:      poliz.emit(Poliz::Op::CMP_GT); break;
            case Token::Type::GreaterEqual: poliz.emit(Poliz::Op::CMP_GE); break;
        }
           }
}

void Parser::parseShift() {
    parseAdditive();

    while (match(Token::Type::Shl) || match(Token::Type::Shr)) {
        finalizeRValue();

        Token op = lex.currentLexeme();
        lex.nextLexem();
        parseAdditive();
        sem.checkBinaryOp(op.type);
        poliz.emit(op.type == Token::Type::Shl
                       ? Poliz::Op::SHL
                       : Poliz::Op::SHR);
    }
}

void Parser::parseAdditive() {
    parseMultiplicative();

    while (match(Token::Type::Plus) ||
           match(Token::Type::Minus)) {
        finalizeRValue();

        Token op = lex.currentLexeme();
        lex.nextLexem();
        parseMultiplicative();
        sem.checkBinaryOp(op.type);
        poliz.emit(
            op.type == Token::Type::Plus
                ? Poliz::Op::ADD
                : Poliz::Op::SUB
        );
    }
}

void Parser::parseMultiplicative() {
    parseUnary();

    while (match(Token::Type::Asterisk) ||
           match(Token::Type::Slash) ||
           match(Token::Type::Percent)) {
        finalizeRValue();

        Token op = lex.currentLexeme();
        lex.nextLexem();
        parseUnary();
        sem.checkBinaryOp(op.type);
        switch (op.type) {
            case Token::Type::Asterisk: poliz.emit(Poliz::Op::MUL); break;
            case Token::Type::Slash:    poliz.emit(Poliz::Op::DIV); break;
            case Token::Type::Percent:  poliz.emit(Poliz::Op::MOD); break;
            default: break;
        }
    }
}

void Parser::parseUnary() {
    if (match(Token::Type::Minus) ||
        match(Token::Type::Exclamation)) {

        Token op = lex.currentLexeme();
        lex.nextLexem();
        parseUnary();
        sem.checkUnaryOp(op.type);

        poliz.emit(
            op.type == Token::Type::Minus
                ? Poliz::Op::NEG
                : Poliz::Op::NOT
        );
        return;
        }
    parsePrimary();
}

void Parser::parsePrimary() {
    if (match(Token::Type::LParen)) {
        lex.nextLexem();
        parseExpression();
        expect(Token::Type::RParen, ")");
        return;
    }

    if (match(Token::Type::Identifier)) {
        Token id = lex.currentLexeme();

        if (lex.peekNextLexeme().type == Token::Type::LParen) {
            lex.nextLexem();
            expect(Token::Type::LParen, "(");

            sem.beginFunctionCall(id.lexeme);

            if (!match(Token::Type::RParen)) {
                do {
                    parseLogicalOr();
                    sem.addCallArg();
                    if (!match(Token::Type::Comma)) break;
                    lex.nextLexem();
                } while (true);
            }

            expect(Token::Type::RParen, ")");

            FunctionSymbol* f = sem.endFunctionCall();
            finalizeRValue();
            poliz.emit(Poliz::Op::CALL, f->polizIndex);

            return;
        }

        parseLValue();

        return;
    }

    parseLiteral();
}

void Parser::parseLiteral() {
    Token tok = lex.currentLexeme();

    TypeInfo t = sem.getLiteralType(tok);
    sem.pushType(t);

    switch (tok.type) {
        case Token::Type::IntegerLiteral:
            poliz.emit(Poliz::Op::PUSH_INT, std::stoi(tok.lexeme));
            break;

        case Token::Type::FloatLiteral: {
            float f = std::stof(tok.lexeme);
            int bits;
            std::memcpy(&bits, &f, sizeof(float));
            poliz.emit(Poliz::Op::PUSH_FLOAT, bits);
            break;
        }

        case Token::Type::CharLiteral:
            poliz.emit(Poliz::Op::PUSH_CHAR, tok.lexeme[1]);
            break;

        case Token::Type::KwTrue:
            poliz.emit(Poliz::Op::PUSH_BOOL, 1);
            break;

        case Token::Type::KwFalse:
            poliz.emit(Poliz::Op::PUSH_BOOL, 0);
            break;

        case Token::Type::StringLiteral: {
            int idx = poliz.addString(tok.lexeme);
            poliz.emit(Poliz::Op::PUSH_STRING, idx);
            break;
        }

        default:
            throw std::runtime_error("expected literal");
    }

    lex.nextLexem();
}

void Parser::parseLValue() {
    Token id = lex.currentLexeme();
    expect(Token::Type::Identifier, "identifier");

    Symbol *sym = sem.lookupVariable(id.lexeme);
    if (!sym)
        throw std::runtime_error("Unknown variable " + id.lexeme);

    LValueDesc lv;
    lv.base = sym;
    lv.kind = LValueDesc::Kind::Var;

    TypeInfo curType = sym->type;

    if (match(Token::Type::LBracket)) {
        lex.nextLexem();
        parseExpression();
        TypeInfo idx = sem.popType();
        sem.checkArrayIndex(curType, idx);
        expect(Token::Type::RBracket, "]");

        lv.kind = LValueDesc::Kind::ArrayElem;
        curType = TypeInfo(curType.baseType);
    }

    lastLValue = lv;
    sem.pushType(curType);
}

void Parser::emitLoadFromLValue(const LValueDesc &lv) {
    switch (lv.kind) {
        case LValueDesc::Kind::Var:
            poliz.emit(Poliz::Op::LOAD_VAR, lv.base->slot);
            break;
        case LValueDesc::Kind::ArrayElem:
            poliz.emit(Poliz::Op::LOAD_ELEM, lv.base->slot);
            break;
    }
}

void Parser::emitStoreToLValue(const LValueDesc &lv) {
    switch (lv.kind) {
        case LValueDesc::Kind::Var:
            poliz.emit(Poliz::Op::STORE_VAR, lv.base->slot);
            break;
        case LValueDesc::Kind::ArrayElem:
            poliz.emit(Poliz::Op::STORE_ELEM, lv.base->slot);
            break;
    }
}

void Parser::finalizeRValue() {
    if (lastLValue) {
        emitLoadFromLValue(*lastLValue);
        lastLValue.reset();
    }
}