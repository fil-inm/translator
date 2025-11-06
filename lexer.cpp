
#include "lexer.hpp"
#include <sstream>
#include <stdexcept>

static Token makeTok(Token::Type t, std::string_view lex, int line, int col) {
    Token tok;
    tok.type = t;
    tok.lexeme = std::string(lex);
    tok.pos = {line, col};
    return tok;
}

Lexer::Lexer(std::string source) : m_src(std::move(source)) {
    kw_ = {
        {"int", Token::Type::KwInt},
        {"char", Token::Type::KwChar},
        {"bool", Token::Type::KwBool},
        {"float", Token::Type::KwFloat},
        {"void", Token::Type::KwVoid},
        {"class", Token::Type::KwClass},
        {"constructor", Token::Type::KwConstructor},
        {"destructor", Token::Type::KwDestructor},
        {"if", Token::Type::KwIf},
        {"elif", Token::Type::KwElif},
        {"else", Token::Type::KwElse},
        {"while", Token::Type::KwWhile},
        {"for", Token::Type::KwFor},
        {"return", Token::Type::KwReturn},
        {"break", Token::Type::KwBreak},
        {"continue", Token::Type::KwContinue},
        {"print", Token::Type::KwPrint},
        {"read", Token::Type::KwRead},
        {"true", Token::Type::KwTrue},
        {"false", Token::Type::KwFalse},
        {"and", Token::Type::KwAnd},
        {"or", Token::Type::KwOr},
        {"new", Token::Type::KwNew},
        {"delete", Token::Type::KwDelete},
    };
    m_lookahead = next();
}

Lexer Lexer::fromFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return Lexer{ss.str()};
}

void Lexer::loadKeywordsFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) return;
    kw_.clear();
    std::string w;
    while (in >> w) {
        if (w == "int") kw_[w] = Token::Type::KwInt;
        else if (w == "char") kw_[w] = Token::Type::KwChar;
        else if (w == "bool") kw_[w] = Token::Type::KwBool;
        else if (w == "float") kw_[w] = Token::Type::KwFloat;
        else if (w == "void") kw_[w] = Token::Type::KwVoid;
        else if (w == "class") kw_[w] = Token::Type::KwClass;
        else if (w == "constructor") kw_[w] = Token::Type::KwConstructor;
        else if (w == "destructor") kw_[w] = Token::Type::KwDestructor;
        else if (w == "if") kw_[w] = Token::Type::KwIf;
        else if (w == "elif") kw_[w] = Token::Type::KwElif;
        else if (w == "else") kw_[w] = Token::Type::KwElse;
        else if (w == "while") kw_[w] = Token::Type::KwWhile;
        else if (w == "for") kw_[w] = Token::Type::KwFor;
        else if (w == "return") kw_[w] = Token::Type::KwReturn;
        else if (w == "break") kw_[w] = Token::Type::KwBreak;
        else if (w == "continue") kw_[w] = Token::Type::KwContinue;
        else if (w == "print") kw_[w] = Token::Type::KwPrint;
        else if (w == "read") kw_[w] = Token::Type::KwRead;
        else if (w == "true") kw_[w] = Token::Type::KwTrue;
        else if (w == "false") kw_[w] = Token::Type::KwFalse;
        else if (w == "and") kw_[w] = Token::Type::KwAnd;
        else if (w == "or") kw_[w] = Token::Type::KwOr;
        else if (w == "new") kw_[w] = Token::Type::KwNew;
        else if (w == "delete") kw_[w] = Token::Type::KwDelete;
    }
}

void Lexer::advance() {
    if (atEnd()) return;
    if (cur() == '\n') { m_line++; m_col = 1; }
    else { m_col++; }
    m_i++;
}

void Lexer::emitEof() {
    m_eof = true;
    m_lookahead = makeTok(Token::Type::EndOfFile, "", m_line, m_col);
}

void Lexer::skipSpacesAndComments() {
    for (;;) {
        while (std::isspace(static_cast<unsigned char>(cur()))) advance();

        if (cur()=='/' && peekChar()=='/') {
            while (!atEnd() && cur()!='\n') advance();
            continue;
        }
        if (cur()=='/' && peekChar()=='*') {
            advance(); advance();
            while (!atEnd() && !(cur()=='*' && peekChar()== '/')) advance();
            if (!atEnd()) { advance(); advance(); }
            continue;
        }
        break;
    }
}

bool Lexer::isIdentStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c=='_';
}
bool Lexer::isIdentCont(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c=='_';
}

Token Lexer::readIdentifierOrKeyword() {
    const int startLine = m_line, startCol = m_col;
    size_t start = m_i;
    while (isIdentCont(cur())) advance();
    std::string word = m_src.substr(start, m_i - start);

    auto it = kw_.find(word);
    if (it != kw_.end()) {
        return makeTok(it->second, word, startLine, startCol);
    }
    return makeTok(Token::Type::Identifier, word, startLine, startCol);
}

Token Lexer::readNumber() {
    const int startLine = m_line, startCol = m_col;
    size_t start = m_i;
    bool seenDot = false;
    while (std::isdigit(static_cast<unsigned char>(cur()))) advance();
    if (cur()=='.' && std::isdigit(static_cast<unsigned char>(peekChar()))) {
        seenDot = true;
        advance();
        while (std::isdigit(static_cast<unsigned char>(cur()))) advance();
    }
    std::string num = m_src.substr(start, m_i - start);
    return makeTok(seenDot ? Token::Type::Float : Token::Type::Integer, num, startLine, startCol);
}

Token Lexer::readCharLiteral() {
    const int startLine = m_line, startCol = m_col;
    advance();
    std::string content;
    if (cur()=='\\') {
        content.push_back(cur()); advance();
        if (!atEnd()) { content.push_back(cur()); advance(); }
    } else if (!atEnd() && cur()!='\'' && cur()!='\n') {
        content.push_back(cur()); advance();
    }
    if (cur()=='\'') advance();
    return makeTok(Token::Type::Char, content, startLine, startCol);
}

Token Lexer::readOperatorOrPunct() {
    const int startLine = m_line, startCol = m_col;
    auto two = std::string() + cur() + peekChar();
    auto three = two + peekChar(2);

    if (three == "<<=") { advance(); advance(); advance(); return makeTok(Token::Type::ShlAssign, three, startLine, startCol); }
    if (three == ">>=") { advance(); advance(); advance(); return makeTok(Token::Type::ShrAssign, three, startLine, startCol); }

    if (two == "++") { advance(); advance(); return makeTok(Token::Type::PlusPlus, two, startLine, startCol); }
    if (two == "--") { advance(); advance(); return makeTok(Token::Type::MinusMinus, two, startLine, startCol); }
    if (two == "==") { advance(); advance(); return makeTok(Token::Type::Eq, two, startLine, startCol); }
    if (two == "!=") { advance(); advance(); return makeTok(Token::Type::Ne, two, startLine, startCol); }
    if (two == "<=") { advance(); advance(); return makeTok(Token::Type::Le, two, startLine, startCol); }
    if (two == ">=") { advance(); advance(); return makeTok(Token::Type::Ge, two, startLine, startCol); }
    if (two == "&&") { advance(); advance(); return makeTok(Token::Type::AmpAmp, two, startLine, startCol); }
    if (two == "||") { advance(); advance(); return makeTok(Token::Type::PipePipe, two, startLine, startCol); }
    if (two == "+=") { advance(); advance(); return makeTok(Token::Type::PlusAssign, two, startLine, startCol); }
    if (two == "-=") { advance(); advance(); return makeTok(Token::Type::MinusAssign, two, startLine, startCol); }
    if (two == "*=") { advance(); advance(); return makeTok(Token::Type::StarAssign, two, startLine, startCol); }
    if (two == "/=") { advance(); advance(); return makeTok(Token::Type::SlashAssign, two, startLine, startCol); }
    if (two == "%=") { advance(); advance(); return makeTok(Token::Type::PercentAssign, two, startLine, startCol); }
    if (two == "<<") { advance(); advance(); return makeTok(Token::Type::Shl, two, startLine, startCol); }
    if (two == ">>") { advance(); advance(); return makeTok(Token::Type::Shr, two, startLine, startCol); }

    char c = cur();
    advance();
    switch (c) {
        case '(': return makeTok(Token::Type::LParen, "(", startLine, startCol);
        case ')': return makeTok(Token::Type::RParen, ")", startLine, startCol);
        case '{': return makeTok(Token::Type::LBrace, "{", startLine, startCol);
        case '}': return makeTok(Token::Type::RBrace, "}", startLine, startCol);
        case '[': return makeTok(Token::Type::LBracket, "[", startLine, startCol);
        case ']': return makeTok(Token::Type::RBracket, "]", startLine, startCol);
        case ',': return makeTok(Token::Type::Comma, ",", startLine, startCol);
        case ';': return makeTok(Token::Type::Semicolon, ";", startLine, startCol);
        case '.': return makeTok(Token::Type::Dot, ".", startLine, startCol);
        case '`': return makeTok(Token::Type::Backtick, "`", startLine, startCol);
        case '+': return makeTok(Token::Type::Plus, "+", startLine, startCol);
        case '-': return makeTok(Token::Type::Minus, "-", startLine, startCol);
        case '*': return makeTok(Token::Type::Star, "*", startLine, startCol);
        case '/': return makeTok(Token::Type::Slash, "/", startLine, startCol);
        case '%': return makeTok(Token::Type::Percent, "%", startLine, startCol);
        case '&': return makeTok(Token::Type::Amp, "&", startLine, startCol);
        case '|': return makeTok(Token::Type::Pipe, "|", startLine, startCol);
        case '^': return makeTok(Token::Type::Caret, "^", startLine, startCol);
        case '!': return makeTok(Token::Type::Bang, "!", startLine, startCol);
        case '~': return makeTok(Token::Type::Tilde, "~", startLine, startCol);
        case '=': return makeTok(Token::Type::Assign, "=", startLine, startCol);
        case '<': return makeTok(Token::Type::Lt, "<", startLine, startCol);
        case '>': return makeTok(Token::Type::Gt, ">", startLine, startCol);
    }
    return makeTok(Token::Type::Identifier, std::string(1,c), startLine, startCol); // fallback
}

Token Lexer::next() {
    if (m_eof) return m_lookahead;

    skipSpacesAndComments();
    if (atEnd()) { emitEof(); return m_lookahead; }

    char c = cur();
    if (isIdentStart(c)) {
        m_lookahead = readIdentifierOrKeyword();
        return m_lookahead;
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
        m_lookahead = readNumber();
        return m_lookahead;
    }
    if (c=='\'') {
        m_lookahead = readCharLiteral();
        return m_lookahead;
    }
    m_lookahead = readOperatorOrPunct();
    return m_lookahead;
}

const Token& Lexer::peek() {
    return m_lookahead;
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> toks;
    while (true) {
        Token t = peek();
        toks.push_back(t);
        if (t.type == Token::Type::EndOfFile) break;
        next();
    }
    return toks;
}
