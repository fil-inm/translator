#pragma once
#include "lexer.hpp"
#include "semanter.hpp"
#include "poliz.hpp"
#include <string>
#include <unordered_set>


struct LValueDesc {
    enum class Kind {
        Var,
        ArrayElem,
        Field
    } kind;

    Symbol* base = nullptr;
    std::string field;
};

struct LoopCtx {
    int start;
    std::vector<int> breaks;
    std::vector<int> continues;
};


class Parser {
public:
    explicit Parser(Lexer& lexer, Semanter& semanter, Poliz& poliz);

    bool parseProgram();

    void parseFunctionDeclaration();

    void parseFunctionDefinition();

private:
    Lexer& lex;
    Semanter& sem;
    Poliz&   poliz;
    int errLine = 0, errCol = 0;

    std::optional<LValueDesc> lastLValue;
    std::vector<LoopCtx> loopStack;


    std::string currentFunctionName;
    bool        hasCurrentFunction = false;
    TypeInfo    currentReturnType;


    bool match(Token::Type t) ;
    bool matchType() ;

    void expect(Token::Type t, const std::string& what) ;

    void parseMain();

    TypeInfo parseType();

    void parseLiteral();
    void parseLValue();

    void parseBlock();
    void parseStatement();

    void parseDeclaration();

    void parseIf();

    void parseWhile();

    void parseFor();

    void parseReturn();

    void parsePrint();

    void parseRead();


    void parseExpression();
    void parseComma();
    void parseAssignment();
    void parseLogicalOr();
    void parseLogicalAnd();
    void parseBitwiseOr();
    void parseBitwiseXor();
    void parseBitwiseAnd();
    void parseEquality();
    void parseRelational();
    void parseShift();
    void parseAdditive();
    void parseMultiplicative();
    void parseUnary();
    void parsePrimary();

    void emitLoadFromLValue(const LValueDesc &lv);

    void emitStoreToLValue(const LValueDesc &lv);

    void finalizeRValue();
};