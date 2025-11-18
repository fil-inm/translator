#pragma once
#include <string>

struct SourcePos {
    int line = 1;
    int column = 1;
};

struct Token {
    enum class Type : uint16_t {
        EndOfFile = 0,
        Identifier,
        IntegerLiteral,
        FloatLiteral,
        CharLiteral,
        StringLiteral,

        LParen, RParen,
        LBrace, RBrace,
        LBracket, RBracket,
        Comma, Semicolon, Dot, Backtick,

        Plus, Minus, Asterisk, Slash, Percent,
        PlusPlus, MinusMinus,

        Assign,

        Shl, Shr,

        Ampersand, VerticalBar, Caret,
        AmpAmp, PipePipe,
        Exclamation, Tilde,

        EqualEqual, NotEqual,
        Less, Greater, LessEqual, GreaterEqual,

        KwInt, KwChar, KwBool, KwFloat, KwVoid,

        KwClass,
        KwConstructor,
        KwMain,

        KwIf, KwElif, KwElse,
        KwWhile, KwFor,
        KwReturn, KwBreak, KwContinue,

        KwPrint, KwRead,

        KwTrue, KwFalse,

        TypeName
    };

    Type type;
    std::string lexeme;
    SourcePos pos;

    std::string toString() const;
};

inline std::string tokenTypeName(Token::Type t) {
    using T = Token::Type;
    switch (t) {
        case T::EndOfFile: return "EOF";
        case T::Identifier: return "identifier";
        case T::IntegerLiteral: return "integer literal";
        case T::FloatLiteral: return "float literal";
        case T::CharLiteral: return "char literal";

        case T::LParen: return "(";
        case T::RParen: return ")";
        case T::LBrace: return "{";
        case T::RBrace: return "}";
        case T::LBracket: return "[";
        case T::RBracket: return "]";
        case T::Comma: return ",";
        case T::Semicolon: return ";";
        case T::Dot: return ".";
        case T::Backtick: return "`";

        case T::Plus: return "+";
        case T::Minus: return "-";
        case T::Asterisk: return "*";
        case T::Slash: return "/";
        case T::Percent: return "%";
        case T::PlusPlus: return "++";
        case T::MinusMinus: return "--";

        case T::Assign: return "=";

        case T::Shl: return "<<";
        case T::Shr: return ">>";

        case T::Ampersand: return "&";
        case T::VerticalBar: return "|";
        case T::Caret: return "^";
        case T::AmpAmp: return "&&";
        case T::PipePipe: return "||";
        case T::Exclamation: return "!";
        case T::Tilde: return "~";

        case T::EqualEqual: return "==";
        case T::NotEqual: return "!=";
        case T::Less: return "<";
        case T::Greater: return ">";
        case T::LessEqual: return "<=";
        case T::GreaterEqual: return ">=";

        case T::KwInt: return "int";
        case T::KwChar: return "char";
        case T::KwBool: return "bool";
        case T::KwFloat: return "float";
        case T::KwVoid: return "void";
        case T::KwClass: return "class";
        case T::KwConstructor: return "constructor";
        case T::KwMain: return "main";
        case T::KwIf: return "if";
        case T::KwElif: return "elif";
        case T::KwElse: return "else";
        case T::KwWhile: return "while";
        case T::KwFor: return "for";
        case T::KwReturn: return "return";
        case T::KwBreak: return "break";
        case T::KwContinue: return "continue";
        case T::KwPrint: return "print";
        case T::KwRead: return "read";
        case T::KwTrue: return "true";
        case T::KwFalse: return "false";



        case T::TypeName: return "<type-name>";
    }
    return "<unknown>";
}

inline std::string Token::toString() const {
    return tokenTypeName(type) + " '" + lexeme + "' @" +
           std::to_string(pos.line) + ":" + std::to_string(pos.column);
}