#include "parser.hpp"
#include <iostream>

Parser::Parser(Lexer &lexer) : lex(lexer) {
}

bool Parser::match(Token::Type t) {
    if (lex.currentLexeme().type == t) {
        lex.nextLexem();
        return true;
    }
    return false;
}

bool Parser::expect(Token::Type t, const std::string &what) {
    if (lex.currentLexeme().type == t) {
        lex.nextLexem();
        return true;
    }
    return error("expected " + what + ", got " + tokenTypeName(lex.currentLexeme().type));
}

bool Parser::error(const std::string &msg) {
    m_error = msg;
    errLine = lex.currentLexeme().pos.line;
    errCol = lex.currentLexeme().pos.column;
    std::cerr << "Parse error at " << errLine << ":" << errCol
            << " — " << msg << "\n";
    return false;
}


bool Parser::parseProgram() {
    // {<class>}
    while (lex.currentLexeme().type == Token::Type::KwClass)
        if (!parseClass()) return false;

    // {<function>}
    while (true) {
        auto t = lex.currentLexeme().type;
        if (t == Token::Type::KwInt || t == Token::Type::KwChar ||
            t == Token::Type::KwBool || t == Token::Type::KwFloat ||
            t == Token::Type::KwVoid || t == Token::Type::Identifier) {
            Token next = lex.peekNextLexeme();
            if (t == Token::Type::KwInt && next.lexeme == "main")
                break;
            if (!parseFunction()) return false;
        } else break;
    }

    // <main>
    if (!parseMain())
        return error("expected 'main' function at end of program");

    // EOF
    if (lex.currentLexeme().type != Token::Type::EndOfFile)
        return error("unexpected tokens after 'main'");

    return true;
}


bool Parser::parseClass() {
    // <class> ::= "class" <identifier> <class_body>
    if (!expect(Token::Type::KwClass, "'class'")) return false;
    if (!expect(Token::Type::Identifier, "class name")) return false;
    if (!parseClassBody()) return false;
    return true;
}

bool Parser::parseClassBody() {
    // <class_body> ::= "{" {<field_declaration>} {<constructor>} <destructor> {<method>} "}"
    if (!expect(Token::Type::LBrace, "'{' to begin class body"))
        return false;

    // {<Field declarations>}
    while (true) {
        Token::Type t = lex.currentLexeme().type;
        if (t == Token::Type::KwInt || t == Token::Type::KwChar ||
            t == Token::Type::KwBool || t == Token::Type::KwFloat ||
            t == Token::Type::KwVoid || t == Token::Type::Identifier) {
            Token next = lex.peekNextLexeme();
            if (next.type == Token::Type::LParen)
                break;
            if (!parseFieldDecl())
                return false;
        } else break;
    }

    // {<Constructors>}
    while (lex.currentLexeme().type == Token::Type::KwConstructor) {
        if (!parseConstructor())
            return false;
    }

    // <Destructor>
    if (!parseDestructor()) {
        return error("expected destructor in class body");
    }

    // {<Methods>}
    while (true) {
        Token::Type t = lex.currentLexeme().type;
        if (t == Token::Type::KwInt || t == Token::Type::KwChar ||
            t == Token::Type::KwBool || t == Token::Type::KwFloat ||
            t == Token::Type::KwVoid || t == Token::Type::Identifier) {
            if (!parseMethod())
                return false;
        } else break;
    }

    // ";"
    if (!expect(Token::Type::RBrace, "'}' to close class body"))
        return false;

    return true;
}

bool Parser::parseFieldDecl() {
    // <field_declaration> ::= <type> <identifier> {("," <identifier>)} ";"

    // <type>
    if (!parseType())
        return error("expected type in field declaration");

    // <identifier>
    if (!expect(Token::Type::Identifier, "field name"))
        return false;

    // {"," <identifier>}
    while (lex.currentLexeme().type == Token::Type::Comma) {
        lex.nextLexem();
        if (!expect(Token::Type::Identifier, "field name after ','"))
            return false;
    }

    // ";"
    if (!expect(Token::Type::Semicolon, "';' at end of field declaration"))
        return false;

    return true;
}

bool Parser::parseConstructor() {
    // <constructor> ::= "constructor" <class_name> <parameters> "{" {<statement>} "}"

    // "constructor"
    if (!expect(Token::Type::KwConstructor, "'constructor'"))
        return false;

    // <class_name>
    if (!expect(Token::Type::Identifier, "class name in constructor"))
        return false;

    // <parameters>
    if (!parseParameters())
        return false;

    // "{"
    if (!expect(Token::Type::LBrace, "'{' to begin constructor body"))
        return false;

    // {<statement>}
    while (lex.currentLexeme().type != Token::Type::RBrace &&
           lex.currentLexeme().type != Token::Type::EndOfFile) {
        if (!parseStatement())
            return false;
    }

    // "}"
    if (!expect(Token::Type::RBrace, "'}' to end constructor body"))
        return false;

    return true;
}

bool Parser::parseDestructor() {
    // <destructor> ::= "destructor" <class_name> "()" "{" {<statement>} "}"

    // "destructor"
    if (!expect(Token::Type::KwDestructor, "'destructor'"))
        return false;

    // <class_name>
    if (!expect(Token::Type::Identifier, "class name in destructor"))
        return false;

    // "()"
    if (!expect(Token::Type::LParen, "'(' after class name"))
        return false;

    if (!expect(Token::Type::RParen, "')' after '(' in destructor"))
        return false;

    // "{"
    if (!expect(Token::Type::LBrace, "'{' to begin destructor body"))
        return false;

    // {<statement>}
    while (lex.currentLexeme().type != Token::Type::RBrace &&
           lex.currentLexeme().type != Token::Type::EndOfFile) {
        if (!parseStatement())
            return false;
    }

    // "}"
    if (!expect(Token::Type::RBrace, "'}' to end destructor body"))
        return false;

    return true;
}

bool Parser::parseMethod() {
    // <method> ::= <type> <identifier> <parameters> "{" {<statement>} "}"

    // <type>
    if (!parseType())
        return error("expected return type in method declaration");

    // <identifier>
    if (!expect(Token::Type::Identifier, "method name"))
        return false;

    // <parameters>
    if (!parseParameters())
        return false;

    // "{"
    if (!expect(Token::Type::LBrace, "'{' to begin method body"))
        return false;

    // {<statement>}
    while (lex.currentLexeme().type != Token::Type::RBrace &&
           lex.currentLexeme().type != Token::Type::EndOfFile) {
        if (!parseStatement())
            return false;
    }

    // "}"
    if (!expect(Token::Type::RBrace, "'}' to end method body"))
        return false;

    return true;
}


bool Parser::parseFunction() {
    // <function> ::= <type> <function_name> <parameters> "{" {<statement>} "}"

    // --- 1. Тип возвращаемого значения ---
    if (!parseType())
        return error("expected return type in function declaration");

    // --- 2. Имя функции ---
    if (!expect(Token::Type::Identifier, "function name"))
        return false;

    // --- 3. Параметры ---
    if (!parseParameters())
        return false;

    // --- 4. Тело функции ---
    if (!expect(Token::Type::LBrace, "'{' to begin function body"))
        return false;

    // --- 5. Последовательность операторов ---
    while (lex.currentLexeme().type != Token::Type::RBrace &&
           lex.currentLexeme().type != Token::Type::EndOfFile) {
        if (!parseStatement())
            return false;
    }

    // --- 6. Конец функции ---
    if (!expect(Token::Type::RBrace, "'}' to end function body"))
        return false;

    return true;
}

bool Parser::parseMain() {
    // <main> ::= "int" "main" <parameters> "{" {<statement>} "}"

    // "int"
    if (!expect(Token::Type::KwInt, "'int' before main"))
        return false;

    // "main"
    if (lex.currentLexeme().type != Token::Type::Identifier ||
        lex.currentLexeme().lexeme != "main") {
        return error("expected function name 'main'");
        }
    lex.nextLexem();

    // <parameters>
    if (!parseParameters())
        return false;

    // "{"
    if (!expect(Token::Type::LBrace, "'{' to begin main body"))
        return false;

    // {<statement>}
    while (lex.currentLexeme().type != Token::Type::RBrace &&
           lex.currentLexeme().type != Token::Type::EndOfFile) {
        if (!parseStatement())
            return false;
           }

    // "}"
    if (!expect(Token::Type::RBrace, "'}' to end main body"))
        return false;

    return true;
}


bool Parser::parseType() {
    // <type> ::= "int" | "char" | "bool" | "float" | "void" | <identifier>
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
            return error("expected type name");
    }
}

bool Parser::parseParameters() {
    // <parameters> ::= "(" <type> <identifier> {("," <type> <identifier>)} ")" | "(" ")"

    // "("
    if (!expect(Token::Type::LParen, "'(' in parameter list")) return false;

    // if ")"
    if (lex.currentLexeme().type == Token::Type::RParen) {
        lex.nextLexem();
        return true;
    }

    // <type>
    if (!parseType()) return false;

    // <identifier>
    if (!expect(Token::Type::Identifier, "parameter name")) return false;

    // {"," <type> <identifier>}
    while (lex.currentLexeme().type == Token::Type::Comma) {
        lex.nextLexem();
        if (!parseType()) return false;
        if (!expect(Token::Type::Identifier, "parameter name")) return false;
    }

    // ")"
    if (!expect(Token::Type::RParen, "')' after parameters")) return false;
    return true;
}

bool Parser::parseExpressionList() {
    // <expression_list> ::= <expression> {("," <expression>)}

    // <expression>
    if (!parseExpression())
        return false;

    // {"," <expression>}
    while (lex.currentLexeme().type == Token::Type::Comma) {
        lex.nextLexem();
        if (!parseExpression())
            return false;
    }

    return true;
}

bool Parser::parseVariableDecl() {
    // ... [= <expression>]

    if (lex.currentLexeme().type == Token::Type::Assign) {
        lex.nextLexem();
        if (!parseExpression())
            return error("expected expression after '=' in variable declaration");
    }
    return true;
}

bool Parser::parseArrayDecl() {
    // ... "[" <integer_literal> "]" [("=" "{" <expression_list> "}")]

    // "["
    if (!expect(Token::Type::LBracket, "'[' after array name"))
        return false;

    // <integer_literal>
    if (lex.currentLexeme().type != Token::Type::IntegerLiteral)
        return error("expected array size (integer literal)");
    lex.nextLexem();

    // "]"
    if (!expect(Token::Type::RBracket, "']' after array size"))
        return false;

    // ["=" "{" <expression_list> "}"]
    if (lex.currentLexeme().type == Token::Type::Assign) {
        lex.nextLexem();
        if (!expect(Token::Type::LBrace, "'{' to start array initializer"))
            return false;

        // <expression_list>
        if (lex.currentLexeme().type != Token::Type::RBrace) {
            if (!parseExpressionList())
                return false;
        }

        if (!expect(Token::Type::RBrace, "'}' to end array initializer"))
            return false;
    }

    return true;
}


bool Parser::parseLiteral() {
    auto t = lex.currentLexeme().type;

    switch (t) {
        case Token::Type::IntegerLiteral:  return parseIntegerLiteral();
        case Token::Type::FloatLiteral:    return parseFloatingLiteral();
        case Token::Type::CharLiteral:     return parseCharacterLiteral();
        case Token::Type::KwTrue:
        case Token::Type::KwFalse:         return parseBooleanLiteral();
        default:
            return error("expected literal (int, float, char, or bool)");
    }
}

bool Parser::parseIntegerLiteral() {
    if (lex.currentLexeme().type == Token::Type::Plus ||
        lex.currentLexeme().type == Token::Type::Minus)
    {
        lex.nextLexem();
    }

    if (lex.currentLexeme().type != Token::Type::IntegerLiteral)
        return error("expected integer literal");

    lex.nextLexem();
    return true;
}

bool Parser::parseFloatingLiteral() {
    if (lex.currentLexeme().type == Token::Type::Plus ||
        lex.currentLexeme().type == Token::Type::Minus)
    {
        lex.nextLexem();
    }

    if (lex.currentLexeme().type != Token::Type::FloatLiteral)
        return error("expected floating-point literal");

    lex.nextLexem();
    return true;
}

bool Parser::parseCharacterLiteral() {
    if (lex.currentLexeme().type != Token::Type::CharLiteral)
        return error("expected character literal");

    lex.nextLexem();
    return true;
}

bool Parser::parseBooleanLiteral() {
    if (lex.currentLexeme().type != Token::Type::KwTrue &&
        lex.currentLexeme().type != Token::Type::KwFalse)
        return error("expected boolean literal ('true' or 'false')");

    lex.nextLexem();
    return true;
}


bool Parser::parseStatement() {
    auto t = lex.currentLexeme().type;

    // глина, которую я могу объяснить
    if (isTypeToken(t)) {
        return parseDeclarationStatement();
    }

    if (t == Token::Type::Identifier) {
        Token next = lex.peekNextLexeme();
        if (next.type == Token::Type::Identifier)
            return parseDeclarationStatement();
    }

    // if
    if (t == Token::Type::KwIf)
        return parseIfStatement();

    // while
    if (t == Token::Type::KwWhile)
        return parseWhileStatement();

    // for
    if (t == Token::Type::KwFor)
        return parseForStatement();

    // return
    if (t == Token::Type::KwReturn)
        return parseReturnStatement();

    // break
    if (t == Token::Type::KwBreak) {
        lex.nextLexem();
        return expect(Token::Type::Semicolon, "';' after break");
    }

    // continue
    if (t == Token::Type::KwContinue) {
        lex.nextLexem();
        return expect(Token::Type::Semicolon, "';' after continue");
    }

    // print / read
    if (t == Token::Type::KwPrint)
        return parsePrintStatement();
    if (t == Token::Type::KwRead)
        return parseReadStatement();

    // Просто выражение
    if (parseExpression())
        return expect(Token::Type::Semicolon, "';' after expression");

    return error("unexpected token '" + lex.currentLexeme().lexeme + "' in statement");
}

bool Parser::parseDeclarationStatement() {
    // <type>
    if (!parseType())
        return error("expected type in variable declaration");
    if (!expect(Token::Type::Identifier, "variable or array name"))
        return false;


    // [ ... ]
    if (lex.currentLexeme().type == Token::Type::LBracket) {
        if (!parseArrayDecl())
            return false;
    } else {
        if (!parseVariableDecl())
            return false;
    }

    // ';'
    if (!expect(Token::Type::Semicolon, "';' after declaration"))
        return false;

    return true;
}


bool Parser::parseIfStatement() {
    if (!expect(Token::Type::KwIf, "'if'"))
        return false;

    if (!expect(Token::Type::LParen, "'(' after 'if'"))
        return false;

    if (!parseExpression())
        return error("expected expression in 'if' condition");

    if (!expect(Token::Type::RParen, "')' after 'if' condition"))
        return false;

    if (!expect(Token::Type::LBrace, "'{' to start 'if' body"))
        return false;

    while (lex.currentLexeme().type != Token::Type::RBrace &&
           lex.currentLexeme().type != Token::Type::EndOfFile)
    {
        if (!parseStatement()) return false;
    }

    if (!expect(Token::Type::RBrace, "'}' to close 'if' body"))
        return false;

    // elif блоки
    while (lex.currentLexeme().type == Token::Type::KwElif) {
        lex.nextLexem();

        if (!expect(Token::Type::LParen, "'(' after 'elif'"))
            return false;

        if (!parseExpression())
            return error("expected expression in 'elif' condition");

        if (!expect(Token::Type::RParen, "')' after 'elif' condition"))
            return false;

        if (!expect(Token::Type::LBrace, "'{' to start 'elif' body"))
            return false;

        while (lex.currentLexeme().type != Token::Type::RBrace &&
               lex.currentLexeme().type != Token::Type::EndOfFile)
        {
            if (!parseStatement()) return false;
        }

        if (!expect(Token::Type::RBrace, "'}' to close 'elif' body"))
            return false;
    }

    // else блок
    if (lex.currentLexeme().type == Token::Type::KwElse) {
        lex.nextLexem();

        if (!expect(Token::Type::LBrace, "'{' to start 'else' body"))
            return false;

        while (lex.currentLexeme().type != Token::Type::RBrace &&
               lex.currentLexeme().type != Token::Type::EndOfFile)
        {
            if (!parseStatement()) return false;
        }

        if (!expect(Token::Type::RBrace, "'}' to close 'else' body"))
            return false;
    }

    return true;
}

bool Parser::parseWhileStatement() {
    if (!expect(Token::Type::KwWhile, "'while'"))
        return false;

    if (!expect(Token::Type::LParen, "'(' after 'while'"))
        return false;

    if (!parseExpression())
        return error("expected expression in 'while' condition");

    if (!expect(Token::Type::RParen, "')' after 'while' condition"))
        return false;

    if (!expect(Token::Type::LBrace, "'{' to start 'while' body"))
        return false;

    while (lex.currentLexeme().type != Token::Type::RBrace &&
           lex.currentLexeme().type != Token::Type::EndOfFile)
    {
        if (!parseStatement()) return false;
    }

    if (!expect(Token::Type::RBrace, "'}' to close 'while' body"))
        return false;

    return true;
}

bool Parser::parseForStatement() {
    if (!expect(Token::Type::KwFor, "'for'"))
        return false;

    if (!expect(Token::Type::LParen, "'(' after for"))
        return false;

    // [for_init]
    if (lex.currentLexeme().type != Token::Type::Semicolon) {
        Token t = lex.currentLexeme();
        Token next = lex.peekNextLexeme();

        // <for_init> ::= <expression> | <variable_declaration>
        if (isTypeToken(t.type) ||
            (t.type == Token::Type::Identifier && next.type == Token::Type::Identifier))
        {
            if (!parseDeclarationStatement())
                return false;
        }
        else {
            if (!parseExpression())
                return false;
            if (!expect(Token::Type::Semicolon, "';' after for-init expression"))
                return false;
        }
    }
    else {
        lex.nextLexem();
    }

    // [<expression>]
    if (lex.currentLexeme().type != Token::Type::Semicolon) {
        if (!parseExpression())
            return false;
    }

    if (!expect(Token::Type::Semicolon, "';' after for-condition"))
        return false;

    // [<expression>]
    if (lex.currentLexeme().type != Token::Type::RParen) {
        if (!parseExpression())
            return false;
    }

    if (!expect(Token::Type::RParen, "')' after for-expression"))
        return false;

    if (!expect(Token::Type::LBrace, "'{' to start for-body"))
        return false;

    while (lex.currentLexeme().type != Token::Type::RBrace &&
           lex.currentLexeme().type != Token::Type::EndOfFile) {
        if (!parseStatement())
            return false;
           }

    return expect(Token::Type::RBrace, "'}' to end for-body");
}

bool Parser::parseReturnStatement() {
    if (!expect(Token::Type::KwReturn, "'return'"))
        return false;

    // Может быть просто "return;"
    if (lex.currentLexeme().type != Token::Type::Semicolon) {
        if (!parseExpression())
            return error("expected expression after 'return'");
    }

    return expect(Token::Type::Semicolon, "';' after 'return'");
}

bool Parser::parsePrintStatement() {
    if (!expect(Token::Type::KwPrint, "'print'"))
        return false;

    if (!expect(Token::Type::LParen, "'(' after 'print'"))
        return false;

    if (!parseExpression())
        return error("expected expression in 'print'");

    if (!expect(Token::Type::RParen, "')' after 'print' argument"))
        return false;

    return expect(Token::Type::Semicolon, "';' after 'print(...)'");
}

bool Parser::parseReadStatement() {
    if (!expect(Token::Type::KwRead, "'read'"))
        return false;

    if (!expect(Token::Type::LParen, "'(' after 'read'"))
        return false;

    if (!expect(Token::Type::Identifier, "identifier in 'read'"))
        return false;

    if (!expect(Token::Type::RParen, "')' after 'read' argument"))
        return false;

    return expect(Token::Type::Semicolon, "';' after 'read(...)'");
}


bool Parser::parseExpression() {
    // <expression> ::= <comma_expression> | <apostrophe_expression>

    if (!parseCommaExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::Backtick) {
        lex.nextLexem();
        if (!parseCommaExpression())
            return error("expected expression after '`'");
    }

    return true;
}

bool Parser::parseCommaExpression() {
    if (!parseAssignmentExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::Comma) {
        lex.nextLexem();
        if (!parseAssignmentExpression())
            return false;
    }
    return true;
}

bool Parser::parseApostropheExpression() {
    if (!parseAssignmentExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::Backtick) {
        lex.nextLexem();
        if (!parseAssignmentExpression())
            return error("expected expression after '`'");
    }
    return true;
}

bool Parser::parseAssignmentExpression() {
    if (!parseLogicalOrExpression())
        return false;

    switch (lex.currentLexeme().type) {
        case Token::Type::Assign:
        case Token::Type::PlusAssign:
        case Token::Type::MinusAssign:
        case Token::Type::Asterisk:
        case Token::Type::SlashAssign:
        case Token::Type::PercentAssign:
        case Token::Type::ShlAssign:
        case Token::Type::ShrAssign:
        case Token::Type::AmpAssign:
        case Token::Type::CaretAssign:
        case Token::Type::PipeAssign:
            lex.nextLexem();
            return parseAssignmentExpression();
        default:
            return true;
    }
}

bool Parser::parseAssignmentLeftExpression() {
    if (lex.currentLexeme().type == Token::Type::Identifier) {
        lex.nextLexem();
        return true;
    }
    if (parseCastExpression())
        return true;
    return parsePostfixExpression();
}

bool Parser::parseLogicalOrExpression() {
    if (!parseLogicalAndExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::PipePipe ||
           lex.currentLexeme().type == Token::Type::KwOr)
    {
        lex.nextLexem();
        if (!parseLogicalAndExpression())
            return error("expected expression after '||' or 'or'");
    }
    return true;
}

bool Parser::parseLogicalAndExpression() {
    if (!parseBitwiseOrExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::AmpAmp ||
           lex.currentLexeme().type == Token::Type::KwAnd)
    {
        lex.nextLexem();
        if (!parseBitwiseOrExpression())
            return error("expected expression after '&&' or 'and'");
    }
    return true;
}

bool Parser::parseBitwiseOrExpression() {
    if (!parseBitwiseXorExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::VerticalBar) {
        lex.nextLexem();
        if (!parseBitwiseXorExpression())
            return error("expected expression after '|'");
    }
    return true;
}

bool Parser::parseBitwiseXorExpression() {
    if (!parseBitwiseAndExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::Caret) {
        lex.nextLexem();
        if (!parseBitwiseAndExpression())
            return error("expected expression after '^'");
    }
    return true;
}

bool Parser::parseBitwiseAndExpression() {
    if (!parseEqualityExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::Ampersand) {
        lex.nextLexem();
        if (!parseEqualityExpression())
            return error("expected expression after '&'");
    }
    return true;
}

bool Parser::parseEqualityExpression() {
    if (!parseRelationalExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::EqualEqual ||
           lex.currentLexeme().type == Token::Type::NotEqual)
    {
        lex.nextLexem();
        if (!parseRelationalExpression())
            return error("expected expression after '==' or '!='");
    }
    return true;
}

bool Parser::parseRelationalExpression() {
    if (!parseShiftExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::Less ||
           lex.currentLexeme().type == Token::Type::Greater ||
           lex.currentLexeme().type == Token::Type::LessEqual ||
           lex.currentLexeme().type == Token::Type::GreaterEqual)
    {
        lex.nextLexem();
        if (!parseShiftExpression())
            return error("expected expression after relational operator");
    }
    return true;
}

bool Parser::parseShiftExpression() {
    if (!parseAdditiveExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::Shl ||
           lex.currentLexeme().type == Token::Type::Shr)
    {
        lex.nextLexem();
        if (!parseAdditiveExpression())
            return error("expected expression after '<<' or '>>'");
    }
    return true;
}

bool Parser::parseAdditiveExpression() {
    if (!parseMultiplicativeExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::Plus ||
           lex.currentLexeme().type == Token::Type::Minus)
    {
        lex.nextLexem();
        if (!parseMultiplicativeExpression())
            return error("expected expression after '+' or '-'");
    }
    return true;
}

bool Parser::parseMultiplicativeExpression() {
    if (!parseCastExpression())
        return false;

    while (lex.currentLexeme().type == Token::Type::Asterisk ||
           lex.currentLexeme().type == Token::Type::Slash ||
           lex.currentLexeme().type == Token::Type::Percent)
    {
        lex.nextLexem();
        if (!parseCastExpression())
            return error("expected expression after '*', '/' or '%'");
    }
    return true;
}

bool Parser::parseCastExpression() {
    if (lex.currentLexeme().type == Token::Type::LParen) {
        Token next = lex.peekNextLexeme();
        if (parseType()) {
            lex.nextLexem();
            lex.nextLexem();
            if (!expect(Token::Type::RParen, "')' after type in cast"))
                return false;
            return parseCastExpression();
        }
    }
    return parseUnaryExpression();
}

bool Parser::parseUnaryExpression() {
    auto t = lex.currentLexeme().type;

    if (t == Token::Type::Slash || t == Token::Type::Asterisk ||
        t == Token::Type::Plus || t == Token::Type::Minus ||
        t == Token::Type::Exclamation || t == Token::Type::Tilde)
    {
        lex.nextLexem();
        return parseCastExpression();
    }

    if (t == Token::Type::PlusPlus || t == Token::Type::MinusMinus) {
        lex.nextLexem();
        return parseCastExpression();
    }

    return parsePostfixExpression();
}

bool Parser::parsePostfixExpression() {
    if (!parsePrimaryExpression())
        return false;

    while (true) {
        auto t = lex.currentLexeme().type;

        if (t == Token::Type::LBracket) {
            lex.nextLexem();
            if (!parseExpression()) return false;
            if (!expect(Token::Type::RBracket, "']'"))
                return false;
        }
        else if (t == Token::Type::Dot) {
            lex.nextLexem();
            if (!expect(Token::Type::Identifier, "identifier after '.'"))
                return false;
        }
        else if (t == Token::Type::PlusPlus || t == Token::Type::MinusMinus) {
            lex.nextLexem();
        }
        else if (t == Token::Type::LParen) {
            // вызов функции
            lex.nextLexem();
            if (lex.currentLexeme().type != Token::Type::RParen) {
                if (!parseExpressionList())
                    return false;
            }
            if (!expect(Token::Type::RParen, "')' after function call"))
                return false;
        }
        else break;
    }

    return true;
}

bool Parser::parsePrimaryExpression() {
    if (lex.currentLexeme().type == Token::Type::Identifier) {
        lex.nextLexem();
        return true;
    }

    if (lex.currentLexeme().type == Token::Type::LParen) {
        lex.nextLexem();
        if (!parseExpression())
            return error("expected expression inside parentheses");
        return expect(Token::Type::RParen, "')' after expression");
    }

    if (lex.currentLexeme().type == Token::Type::IntegerLiteral ||
        lex.currentLexeme().type == Token::Type::FloatLiteral ||
        lex.currentLexeme().type == Token::Type::CharLiteral ||
        lex.currentLexeme().type == Token::Type::KwTrue ||
        lex.currentLexeme().type == Token::Type::KwFalse)
    {
        return parseLiteral();
    }

    return error("expected primary expression (identifier, literal, or (expr))");
}

bool Parser::isTypeToken(Token::Type t) {
    switch (t) {
        case Token::Type::KwInt:
        case Token::Type::KwChar:
        case Token::Type::KwBool:
        case Token::Type::KwFloat:
        case Token::Type::KwVoid:
        case Token::Type::TypeName:
            return true;
        default:
            return false;
    }
}