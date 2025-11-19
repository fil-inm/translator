#pragma once
#include "lexer.hpp"
#include <string>
#include <unordered_set>

class Parser {
public:
    explicit Parser(Lexer& lexer);

    bool parseProgram();

private:
    Lexer& lex;
    int errLine = 0, errCol = 0;

    bool match(Token::Type t) const;
    bool matchType() const;

    bool expect(Token::Type t, const std::string& what) const;


    bool parseClass();
    bool parseClassBody();
    bool parseConstructor();

    bool parseFunction();
    bool parseParameters();
    bool parseMain();

    bool parseType() const;
    bool parseLiteral();
    bool parseLValue();
    bool parseLValueMethod();

    bool parseBlock();
    bool parseStatement();
    bool parseDeclarationStatement();
    bool parseIfStatement();
    bool parseWhileStatement();
    bool parseForStatement();
    bool parseReturnStatement();
    bool parsePrintStatement();
    bool parseReadStatement();

    bool parseExpression();
    bool parseCommaExpression();
    // bool parseApostropheExpression();
    bool parseAssignmentExpression();
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
    bool parseUnaryExpression();
    bool parsePrimaryExpression();
};