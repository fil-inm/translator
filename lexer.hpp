#pragma once
#include "tokens.hpp"
#include "trie.hpp"
#include <string>
#include <fstream>


class Lexer {
public:

    explicit Lexer(const std::string &filename, std::string keywordFile = "keywords.txt");
    void loadKeywordsFromFile(const std::string& filename);

    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    Lexer(Lexer&& other) noexcept;
    Lexer& operator=(Lexer&& other) noexcept;

    const Token& currentLexeme() const;

    Token nextLexem();
    Token peekNextLexeme();

private:
    std::ifstream file;
    Trie keywords;
    char currentChar = '\0';
    bool eof = false;
    int line = 1;
    int column = 0;

    Token currentToken;

    void readChar();
    void skipWhitespaceAndComments();
    static Token makeToken(Token::Type type, const std::string& value, int line, int col);

    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readCharLiteral();

    Token readStringLiteral();

    Token readOperatorOrDelimiter();
};
