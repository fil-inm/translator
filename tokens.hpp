#pragma once
#include <string>

enum class TokenType {

    UNKNOWN           = 0,
    END_OF_FILE       = 1,

    IDENTIFIER        = 100,
    KEYWORD           = 101,
    TYPE_NAME         = 102,

    NUMBER            = 200,
    STRING_LITERAL    = 201,
    CHAR_LITERAL      = 202,
    BOOL_LITERAL      = 203,

    OPERATOR_PLUS     = 300,
    OPERATOR_MINUS    = 301,
    OPERATOR_MUL      = 302,
    OPERATOR_DIV      = 303,
    OPERATOR_ASSIGN   = 304,
    OPERATOR_EQ       = 305,
    OPERATOR_NEQ      = 306,
    OPERATOR_LT       = 307,
    OPERATOR_GT       = 308,
    OPERATOR_LE       = 309,
    OPERATOR_GE       = 310,
    OPERATOR_AND      = 311,
    OPERATOR_OR       = 312,
    OPERATOR_NOT      = 313,
    OPERATOR_INC      = 314,
    OPERATOR_DEC      = 315,
    OPERATOR_SCOPE    = 316,
    OPERATOR_MOD      = 317,

    DELIM_SEMICOLON   = 400,
    DELIM_COMMA       = 401,
    DELIM_DOT         = 402,
    DELIM_LPAREN      = 403,
    DELIM_RPAREN      = 404,
    DELIM_LBRACE      = 405,
    DELIM_RBRACE      = 406,
    DELIM_LBRACKET    = 407,
    DELIM_RBRACKET    = 408,
    DELIM_COLON       = 409,


    COMMENT           = 500
};

struct Token {
    TokenType type;
    std::string value;
};