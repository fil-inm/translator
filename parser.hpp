#pragma once
#include "lexer.hpp"
#include <string>
#include <unordered_set>

class Parser {
public:
    explicit Parser(Lexer& lexer);

    bool parseProgram();
    const std::string& lastError() const { return m_error; }

private:
    Lexer& lex;
    std::string m_error;
    int errLine = 0, errCol = 0;

    bool match(Token::Type t);
    bool expect(Token::Type t, const std::string& what);
    bool error(const std::string& msg);

    bool parseProgram_Classes();
    bool parseProgram_Functions();
    bool parseMain();

    bool parseClass();
    bool parseClassBody();
    bool parseFieldDecl();
    bool parseConstructor();
    bool parseDestructor();
    bool parseMethod();

    bool parseFunction();

    bool parseType();
    bool parseParameters();
    bool parseExpressionList();
    bool parseVariableDecl();
    bool parseArrayDecl();

    bool parseStatement();
    bool parseDeclarationStatement();
    bool parseIfStatement();
    bool parseWhileStatement();
    bool parseForStatement();
    bool parseReturnStatement();
    bool parseBreakStatement();
    bool parseContinueStatement();
    bool parsePrintStatement();
    bool parseReadStatement();

    bool parseExpression();
    bool parseCommaExpression();
    bool parseApostropheExpression();
    bool parseAssignmentExpression();
    bool parseAssignmentLeftExpression();
    bool parseLogicalOrExpression();
    bool parseLogicalAndExpression();
    bool parseBitwiseOrExpression();
    bool parseBitwiseXorExpression();
    bool parseBitwiseAndExpression();
    bool parseEqualityExpression();
    bool parseRelationalExpression();
    bool parseShiftExpression();
    bool parseAdditiveExpression();
    bool parseMultiplicativeExpression();
    bool parseCastExpression();
    bool parseUnaryExpression();
    bool parsePostfixExpression();
    bool parsePrimaryExpression();

    bool isTypeToken(Token::Type t);

    bool parseLiteral();
    bool parseIntegerLiteral();
    bool parseFloatingLiteral();
    bool parseCharacterLiteral();
    bool parseBooleanLiteral();
};