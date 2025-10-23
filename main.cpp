#include "lexer.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD:          return "KEYWORD";
        case TokenType::IDENTIFIER:       return "IDENTIFIER";
        case TokenType::TYPE_NAME:        return "TYPE_NAME";

        case TokenType::NUMBER:           return "NUMBER";
        case TokenType::STRING_LITERAL:   return "STRING_LITERAL";
        case TokenType::CHAR_LITERAL:     return "CHAR_LITERAL";
        case TokenType::BOOL_LITERAL:     return "BOOL_LITERAL";

        case TokenType::OPERATOR_PLUS:    return "OPERATOR_PLUS";
        case TokenType::OPERATOR_MINUS:   return "OPERATOR_MINUS";
        case TokenType::OPERATOR_MUL:     return "OPERATOR_MUL";
        case TokenType::OPERATOR_DIV:     return "OPERATOR_DIV";
        case TokenType::OPERATOR_ASSIGN:  return "OPERATOR_ASSIGN";
        case TokenType::OPERATOR_EQ:      return "OPERATOR_EQ";
        case TokenType::OPERATOR_NEQ:     return "OPERATOR_NEQ";
        case TokenType::OPERATOR_LT:      return "OPERATOR_LT";
        case TokenType::OPERATOR_GT:      return "OPERATOR_GT";
        case TokenType::OPERATOR_LE:      return "OPERATOR_LE";
        case TokenType::OPERATOR_GE:      return "OPERATOR_GE";
        case TokenType::OPERATOR_AND:     return "OPERATOR_AND";
        case TokenType::OPERATOR_OR:      return "OPERATOR_OR";
        case TokenType::OPERATOR_NOT:     return "OPERATOR_NOT";
        case TokenType::OPERATOR_INC:     return "OPERATOR_INC";
        case TokenType::OPERATOR_DEC:     return "OPERATOR_DEC";

        case TokenType::DELIM_SEMICOLON:  return "DELIM_SEMICOLON";
        case TokenType::DELIM_COMMA:      return "DELIM_COMMA";
        case TokenType::DELIM_DOT:        return "DELIM_DOT";
        case TokenType::DELIM_LPAREN:     return "DELIM_LPAREN";
        case TokenType::DELIM_RPAREN:     return "DELIM_RPAREN";
        case TokenType::DELIM_LBRACE:     return "DELIM_LBRACE";
        case TokenType::DELIM_RBRACE:     return "DELIM_RBRACE";
        case TokenType::DELIM_LBRACKET:   return "DELIM_LBRACKET";
        case TokenType::DELIM_RBRACKET:   return "DELIM_RBRACKET";

        case TokenType::END_OF_FILE:      return "END_OF_FILE";
        default:                          return "UNKNOWN";
    }
}

int main() {
    Lexer lexer("input.txt");
    lexer.loadKeywordsFromFile("keywords.txt");

    std::vector<Token> tokens;

    while (true) {
        Token token = lexer.nextLexem();
        if (token.type == TokenType::END_OF_FILE)
            break;
        tokens.push_back(token);
    }

    std::ofstream out("tokens.txt");
    if (!out.is_open()) {
        std::cerr << "❌ Cannot open tokens.txt for writing.\n";
        return 1;
    }

    out << "-------------------------------------------------------------\n";
    out << "| #  | TOKEN TYPE        | VALUE        | LINE | COLUMN |\n";
    out << "-------------------------------------------------------------\n";

    int index = 1;
    for (const auto& t : tokens) {
        out << "| "
            << std::setw(2) << index++ << " "
            << "| " << std::left << std::setw(17) << tokenTypeToString(t.type)
            << "| " << std::left << std::setw(12) << t.value
            << "| " << std::setw(5) << t.line
            << "| " << std::setw(7) << t.column << "|\n";
    }

    out << "-------------------------------------------------------------\n";
    out.close();

    std::cout << "✅ Tokens written successfully!\n";
    return 0;
}