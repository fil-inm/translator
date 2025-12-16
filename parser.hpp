#pragma once
#include "lexer.hpp"
#include "semanter.hpp"
#include "poliz.hpp"
#include <string>
#include <unordered_set>


struct LValueDesc {
    enum class Kind {
        Var,        // обычная переменная
        ArrayElem,  // a[i]
        Field       // a.b
    } kind;

    SymbolEntry* base;      // переменная a
    std::string  field;    // имя поля (если Field)
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

    std::string currentClassName;
    bool        inClass = false;
    bool currentIsMethod = false;

    bool match(Token::Type t) ;
    bool matchType() ;

    bool expect(Token::Type t, const std::string& what) ;


    bool parseClass();
    bool parseClassBody();
    bool parseConstructor();

    bool parseFunction();
    bool parseParameters();
    bool parseMain();

    bool parseType() ;
    TypeInfo parseTypeSemantic();

    bool parseLiteral();
    bool parseLValue();

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

    void emitLoadFromLValue(const LValueDesc &lv);

    void emitStoreToLValue(const LValueDesc &lv);
};