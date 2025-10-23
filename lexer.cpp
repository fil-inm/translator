#include "lexer.hpp"
#include <cctype>
#include <iostream>
#include <unordered_map>

Lexer::Lexer(const std::string &filename) {
    file.open(filename);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open " << filename << std::endl;
        eof = true;
        return;
    }
    readChar();
}

void Lexer::readChar() {
    if (!file.get(currentChar)) {
        eof = true;
        currentChar = '\0';
    }
}

void Lexer::skipWhitespace() {
    while (std::isspace(currentChar))
        readChar();
}

void Lexer::loadKeywordsFromFile(const std::string &filename) {
    std::ifstream kwFile(filename);
    if (!kwFile.is_open()) {
        std::cerr << "Error: cannot open keywords file: " << filename << std::endl;
        return;
    }

    std::string word;
    while (kwFile >> word)
        keywords.insert(word);

    std::cout << "Loaded keywords from " << filename << std::endl;
}

Token Lexer::makeToken(TokenType type, const std::string &value) {
    return {type, value};
}

Token Lexer::nextLexem() {
    skipWhitespace();

    if (eof)
        return makeToken(TokenType::END_OF_FILE, "");

    if (std::isalpha(currentChar) || currentChar == '_') {
        std::string word;
        while (std::isalnum(currentChar) || currentChar == '_') {
            word += currentChar;
            readChar();
        }

        if (word == "true" || word == "false")
            return makeToken(TokenType::BOOL_LITERAL, word);

        if (keywords.search(word)) {
            if (word == "int" || word == "float" || word == "double" || word == "char" || word == "bool" || word ==
                "void")
                return makeToken(TokenType::TYPE_NAME, word);
            return makeToken(TokenType::KEYWORD, word);
        }

        return makeToken(TokenType::IDENTIFIER, word);
    }


    if (std::isdigit(currentChar)) {
        std::string num;
        while (std::isdigit(currentChar) || currentChar == '.') {
            num += currentChar;
            readChar();
        }
        return makeToken(TokenType::NUMBER, num);
    }

    if (currentChar == '"') {
        std::string str;
        readChar();
        while (!eof && currentChar != '"') {
            str += currentChar;
            readChar();
        }
        readChar();
        return makeToken(TokenType::STRING_LITERAL, str);
    }

    if (currentChar == '\'') {
        readChar();
        std::string ch(1, currentChar);
        readChar();
        readChar();
        return makeToken(TokenType::CHAR_LITERAL, ch);
    }

    std::string op(1, currentChar);
    readChar();

    if (!eof) {
        std::string two = op + currentChar;
        static const std::unordered_map<std::string, TokenType> multiOps = {
            {"==", TokenType::OPERATOR_EQ},
            {"!=", TokenType::OPERATOR_NEQ},
            {"<=", TokenType::OPERATOR_LE},
            {">=", TokenType::OPERATOR_GE},
            {"++", TokenType::OPERATOR_INC},
            {"--", TokenType::OPERATOR_DEC},
            {"&&", TokenType::OPERATOR_AND},
            {"||", TokenType::OPERATOR_OR},
            {"::", TokenType::OPERATOR_SCOPE}
        };
        auto it = multiOps.find(two);
        if (it != multiOps.end()) {
            readChar();
            return makeToken(it->second, two);
        }
    }


    static const std::unordered_map<std::string, TokenType> singleOps = {
        {"+", TokenType::OPERATOR_PLUS},
        {"-", TokenType::OPERATOR_MINUS},
        {"*", TokenType::OPERATOR_MUL},
        {"/", TokenType::OPERATOR_DIV},
        {"%", TokenType::OPERATOR_MOD},
        {"=", TokenType::OPERATOR_ASSIGN},
        {"<", TokenType::OPERATOR_LT},
        {">", TokenType::OPERATOR_GT},
        {"!", TokenType::OPERATOR_NOT},

        {";", TokenType::DELIM_SEMICOLON},
        {",", TokenType::DELIM_COMMA},
        {".", TokenType::DELIM_DOT},
        {"(", TokenType::DELIM_LPAREN},
        {")", TokenType::DELIM_RPAREN},
        {"{", TokenType::DELIM_LBRACE},
        {"}", TokenType::DELIM_RBRACE},
        {"[", TokenType::DELIM_LBRACKET},
        {"]", TokenType::DELIM_RBRACKET},
        {":", TokenType::DELIM_COLON}
    };

    auto it = singleOps.find(op);
    if (it != singleOps.end())
        return makeToken(it->second, op);

    std::cerr << "Unknown token: '" << op << "' (next char: '" << currentChar << "')\n";

    return makeToken(TokenType::UNKNOWN, op);
}
