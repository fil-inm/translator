#pragma once
#include "tokens.hpp"
#include <fstream>

class Lexer {
public:
    explicit Lexer(std::string source);
    static Lexer fromFile(const std::string& path);
    void loadKeywordsFromFile(const std::string& path);

    Token next();
    const Token& peek();
    bool eof() const { return m_eof; }

    std::vector<Token> tokenizeAll();

private:
    std::string m_src;
    size_t m_i = 0;
    int m_line = 1;
    int m_col = 1;
    bool m_eof = false;
    Token m_lookahead{Token::Type::EndOfFile, "", {1,1}};

    std::unordered_map<std::string, Token::Type> kw_;

    bool atEnd() const { return m_i >= m_src.size(); }
    char cur() const { return atEnd() ? '\0' : m_src[m_i]; }
    char peekChar(int k=1) const { return (m_i + (size_t)k < m_src.size()) ? m_src[m_i+k] : '\0'; }
    void advance();
    void emitEof();

    void skipSpacesAndComments();
    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readCharLiteral();
    Token readOperatorOrPunct();

    static bool isIdentStart(char c);
    static bool isIdentCont(char c);
};
