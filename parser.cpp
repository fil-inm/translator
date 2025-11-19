#include "parser.hpp"
#include <iostream>

Parser::Parser(Lexer &lexer) : lex(lexer) {
}

bool Parser::match(Token::Type t) const {
    return lex.currentLexeme().type == t;
}

bool Parser::matchType() const {
    static const std::unordered_set<Token::Type> types = {
        Token::Type::KwInt,
        Token::Type::KwFloat,
        Token::Type::KwVoid,
        Token::Type::KwChar,
        Token::Type::KwBool,
        Token::Type::TypeName
    };

    return types.contains(lex.currentLexeme().type);
}

bool Parser::expect(Token::Type t, const std::string &what) const {
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
            throw std::runtime_error("expected main function");

        if (!match(Token::Type::EndOfFile))
            throw std::runtime_error("unexpected tokens after 'main'");

        return true;
    } catch (const std::exception &e) {
        errLine = lex.currentLexeme().pos.line;
        errCol = lex.currentLexeme().pos.column;
        std::cerr << "\033[1;31m"
                << "Error at " << errLine << ":" << errCol << "\n"
                << e.what()
                << "\033[0m\n";
        return false;
    }
}


bool Parser::parseClass() {
    expect(Token::Type::KwClass, "'class'");
    expect(Token::Type::Identifier, "class name");
    return parseClassBody();
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
    expect(Token::Type::Identifier, "class name in constructor");

    if (!parseParameters())
        return false;

    if (!parseBlock())
        return false;

    return true;
}


bool Parser::parseFunction() {
    if (!parseType())
        return false;

    expect(Token::Type::Identifier, "function name");

    if (!parseParameters())
        return false;

    if (!parseBlock())
        return false;

    return true;
}

bool Parser::parseParameters() {
    expect(Token::Type::LParen, "'(' in parameter list");

    if (match(Token::Type::RParen)) {
        lex.nextLexem();
        return true;
    }

    if (!parseType()) return false;

    expect(Token::Type::Identifier, "parameter name");

    while (match(Token::Type::Comma)) {
        lex.nextLexem();
        if (!parseType()) return false;
        expect(Token::Type::Identifier, "parameter name");
    }

    expect(Token::Type::RParen, "')' after parameters");
    return true;
}

bool Parser::parseMain() {
    expect(Token::Type::KwMain, "'main'");

    if (!parseParameters())
        return false;

    if (!parseBlock())
        return false;

    return true;
}


bool Parser::parseType() const {
    switch (lex.currentLexeme().type) {
        case Token::Type::KwInt:
        case Token::Type::KwChar:
        case Token::Type::KwBool:
        case Token::Type::KwFloat:
        case Token::Type::KwVoid:
        case Token::Type::Identifier:
            lex.nextLexem();
            return true;
        default:
            throw std::runtime_error("expected type");
    }
}

bool Parser::parseLiteral() {
    if (match(Token::Type::IntegerLiteral)) {
        lex.nextLexem();
        return true;
    }

    if (match(Token::Type::FloatLiteral)) {
        lex.nextLexem();
        return true;
    }

    if (match(Token::Type::CharLiteral)) {
        lex.nextLexem();
        return true;
    }

    if (match(Token::Type::StringLiteral)) {
        lex.nextLexem();
        return true;
    }

    if (match(Token::Type::KwTrue) || match(Token::Type::KwFalse)) {
        lex.nextLexem();
        return true;
    }

    throw std::runtime_error("expected literal (int, float, char, string, or bool)");
}

bool Parser::parseLValue() {
    expect(Token::Type::Identifier, "'lvalue'");
    if (match(Token::Type::LBracket)) {
        lex.nextLexem();
        if (!parseLValueMethod())
            return false;
        expect(Token::Type::RBracket, "close of array");
        return true;
    }
    return true;
}

bool Parser::parseLValueMethod() {
    expect(Token::Type::Identifier, "lvalue");
    if (match(Token::Type::LBracket)) {
        lex.nextLexem();
        if (!parseLValueMethod())
            return false;
        expect(Token::Type::RBracket, "close of array");
        return true;
    } else if (match(Token::Type::Dot)) {
        lex.nextLexem();
        expect(Token::Type::Identifier, "method name");
    } else if (match(Token::Type::LParen)) {
        lex.nextLexem();
        if (!parseExpression())
            return false;
        while (match(Token::Type::Comma)) {
            lex.nextLexem();
            if (!parseExpression())
                return false;
        }
        expect(Token::Type::RParen, "close of function");
    }
    return true;
}


bool Parser::parseBlock() {
    expect(Token::Type::LBrace, "'{' to begin block");

    while (!match(Token::Type::RBrace)) {
        parseStatement();
    }

    expect(Token::Type::RBrace, "'}' to close block");

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
    expect(Token::Type::Identifier, "variable or array name");

    if (match(Token::Type::LBracket)) {
        expect(Token::Type::IntegerLiteral, "expected integer size of array");
        expect(Token::Type::RBracket, "expected ] after size of array");
        return true;
    }
    expect(Token::Type::Semicolon, "expected ; after decloration");

    return true;
}

bool Parser::parseIfStatement() {
    expect(Token::Type::KwIf, "'if'");
    expect(Token::Type::LParen, "'(' after 'if'");

    if (!parseExpression())
        return false;

    expect(Token::Type::RParen, "')' after 'if' condition");

    if (!parseBlock())
        return false;

    if (match(Token::Type::KwElse)) {
        lex.nextLexem();
        if (match(Token::Type::KwIf)) {
            if (!parseIfStatement())
                return false;
        } else {
            if (!parseBlock()) {
                return false;
            }
        }
    }

    return true;
}

bool Parser::parseWhileStatement() {
    expect(Token::Type::KwWhile, "'while'");

    expect(Token::Type::LParen, "'(' after 'while'");

    if (!parseExpression())
        return false;

    expect(Token::Type::RParen, "')' after 'while' condition");

    if (!parseBlock())
        return false;

    return true;
}

bool Parser::parseForStatement() {
    expect(Token::Type::KwFor, "'for'");
    expect(Token::Type::LParen, "'(' after for");

    if (!parseType())
        return false;

    expect(Token::Type::Identifier, "name of variable");
    expect(Token::Type::Assign, "sign =");

    if (!parseExpression())
        return false;

    expect(Token::Type::Semicolon, "';' after init expression");

    if (!parseExpression())
        return false;

    expect(Token::Type::Semicolon, "';' after logic expression");

    if (!parseExpression())
        return false;

    expect(Token::Type::Semicolon, "';' after change expression");

    return true;
}

bool Parser::parseReturnStatement() {
    expect(Token::Type::KwReturn, "'return'");

    if (match(Token::Type::Semicolon)) {
        return true;
    }

    if (!parseExpression())
        return false;

    expect(Token::Type::Semicolon, "';' after return statement");

    return true;
}

bool Parser::parsePrintStatement() {
    expect(Token::Type::KwPrint, "'print'");
    expect(Token::Type::LParen, "'(' after 'print'");

    if (!parseExpression())
        return false;

    expect(Token::Type::RParen, "')' after 'print' argument");
    expect(Token::Type::Semicolon, "';' after 'print(...)'");
    return true;
}

bool Parser::parseReadStatement() {
    expect(Token::Type::KwRead, "'read'");
    expect(Token::Type::LParen, "'(' after 'read'");

    if (!parseLValueMethod())
        return false;

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
    if (match(Token::Type::Identifier)) {
        if (!parseLValue())
            return false;
        expect(Token::Type::Assign, "= after lvalue");
    }

    if (!parseLogicalOrExpression())
        return false;

    return true;
}

bool Parser::parseLogicalOrExpression() {
    if (!parseLogicalAndExpression())
        return false;

    while (match(Token::Type::PipePipe)) {
        lex.nextLexem();
        if (!parseLogicalAndExpression())
            return false;
    }
    return true;
}

bool Parser::parseLogicalAndExpression() {
    if (!parseBitwiseOrExpression())
        return false;

    while (match(Token::Type::AmpAmp)) {
        lex.nextLexem();
        if (!parseBitwiseOrExpression())
            return false;
    }
    return true;
}

bool Parser::parseBitwiseOrExpression() {
    if (!parseBitwiseXorExpression())
        return false;

    while (match(Token::Type::VerticalBar)) {
        lex.nextLexem();
        if (!parseBitwiseXorExpression())
            return false;
    }
    return true;
}

bool Parser::parseBitwiseXorExpression() {
    if (!parseBitwiseAndExpression())
        return false;

    while (match(Token::Type::Caret)) {
        lex.nextLexem();
        if (!parseBitwiseAndExpression())
            return false;
    }
    return true;
}

bool Parser::parseBitwiseAndExpression() {
    if (!parseEqualityExpression())
        return false;

    while (match(Token::Type::Ampersand)) {
        lex.nextLexem();
        if (!parseEqualityExpression())
            return false;
    }
    return true;
}

bool Parser::parseEqualityExpression() {
    if (!parseRelationalExpression())
        return false;

    while (match(Token::Type::EqualEqual) || match(Token::Type::NotEqual)) {
        lex.nextLexem();
        if (!parseRelationalExpression())
            return false;
    }
    return true;
}

bool Parser::parseRelationalExpression() {
    if (!parseShiftExpression())
        return false;

    while (match(Token::Type::Less) || match(Token::Type::Greater) || match(Token::Type::LessEqual) || match(
               Token::Type::GreaterEqual)) {
        lex.nextLexem();
        if (!parseShiftExpression())
            return false;
    }
    return true;
}

bool Parser::parseShiftExpression() {
    if (!parseAdditiveExpression())
        return false;

    while (match(Token::Type::Shl) || match(Token::Type::Shr)) {
        lex.nextLexem();
        if (!parseAdditiveExpression())
            return false;
    }
    return true;
}

bool Parser::parseAdditiveExpression() {
    if (!parseMultiplicativeExpression())
        return false;

    while (match(Token::Type::Plus) || match(Token::Type::Minus)) {
        lex.nextLexem();
        if (!parseMultiplicativeExpression())
            return false;
    }
    return true;
}

bool Parser::parseMultiplicativeExpression() {
    if (!parseUnaryExpression())
        return false;

    while (match(Token::Type::Asterisk) || match(Token::Type::Slash) || match(Token::Type::Percent)) {
        lex.nextLexem();
        if (!parseUnaryExpression())
            return false;
    }
    return true;
}

bool Parser::parseUnaryExpression() {
    if (match(Token::Type::MinusMinus) || match(Token::Type::PlusPlus ) || match(Token::Type::Minus ) || match(Token::Type::Exclamation) || match(Token::Type::Tilde)) {
        lex.nextLexem();
        while (match(Token::Type::MinusMinus) || match(Token::Type::PlusPlus ) || match(Token::Type::Minus ) || match(Token::Type::Exclamation) || match(Token::Type::Tilde)) {
            lex.nextLexem();
        }
        if (!parsePrimaryExpression())
            return false;
    } else if (!parseLiteral())
        return false;

    return true;
}

bool Parser::parsePrimaryExpression() {
    if (match(Token::Type::LParen)) {
        lex.nextLexem();
        if (!parseExpression())
            return false;
        expect(Token::Type::RParen, "')' after expression");
    }
    else if (!parseLValueMethod())
        return false;

    return true;
}
