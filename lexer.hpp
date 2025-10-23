#pragma once
#include "tokens.hpp"
#include "trie.hpp"
#include <string>
#include <fstream>

class Lexer {
private:
    std::ifstream file;
    Trie keywords;
    char currentChar;
    bool eof = false;
    int line;
    int column;

    void readChar();
    void skipWhitespace();
    Token makeToken(TokenType type, const std::string& value);

public:
    Lexer(const std::string& filename);
    void loadKeywordsFromFile(const std::string& filename);
    Token nextLexem();
};