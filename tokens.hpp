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
        Integer,
        Float,
        Char,

        LParen, RParen,
        LBrace, RBrace,
        LBracket, RBracket,
        Comma, Semicolon, Dot, Backtick,

        Plus, Minus, Star, Slash, Percent,
        PlusPlus, MinusMinus,
        Assign, PlusAssign, MinusAssign, StarAssign, SlashAssign, PercentAssign,
        Shl, Shr, ShlAssign, ShrAssign,
        Amp, Pipe, Caret,
        AmpAmp, PipePipe,
        Bang, Tilde,
        Eq, Ne, Lt, Gt, Le, Ge,

        KwInt, KwChar, KwBool, KwFloat, KwVoid,
        KwClass,
        KwConstructor, KwDestructor,
        KwIf, KwElif, KwElse,
        KwWhile, KwFor,
        KwReturn, KwBreak, KwContinue,
        KwPrint, KwRead,
        KwTrue, KwFalse,
        KwAnd, KwOr,
        KwNew, KwDelete,

        TypeName
    };

    Type type;
    std::string lexeme;
    SourcePos pos;

    std::string toString() const;
};

inline std::string tokenTypeName(Token::Type t) {
    switch (t) {
        case Token::Type::EndOfFile: return "EOF";
        case Token::Type::Identifier: return "Identifier";
        case Token::Type::Integer: return "Integer";
        case Token::Type::Float: return "Float";
        case Token::Type::Char: return "Char";

        case Token::Type::LParen: return "(";
        case Token::Type::RParen: return ")";
        case Token::Type::LBrace: return "{";
        case Token::Type::RBrace: return "}";
        case Token::Type::LBracket: return "[";
        case Token::Type::RBracket: return "]";
        case Token::Type::Comma: return ",";
        case Token::Type::Semicolon: return ";";
        case Token::Type::Dot: return ".";
        case Token::Type::Backtick: return "`";

        case Token::Type::Plus: return "+";
        case Token::Type::Minus: return "-";
        case Token::Type::Star: return "*";
        case Token::Type::Slash: return "/";
        case Token::Type::Percent: return "%";
        case Token::Type::PlusPlus: return "++";
        case Token::Type::MinusMinus: return "--";
        case Token::Type::Assign: return "=";
        case Token::Type::PlusAssign: return "+=";
        case Token::Type::MinusAssign: return "-=";
        case Token::Type::StarAssign: return "*=";
        case Token::Type::SlashAssign: return "/=";
        case Token::Type::PercentAssign: return "%=";
        case Token::Type::Shl: return "<<";
        case Token::Type::Shr: return ">>";
        case Token::Type::ShlAssign: return "<<=";
        case Token::Type::ShrAssign: return ">>=";
        case Token::Type::Amp: return "&";
        case Token::Type::Pipe: return "|";
        case Token::Type::Caret: return "^";
        case Token::Type::AmpAmp: return "&&";
        case Token::Type::PipePipe: return "||";
        case Token::Type::Bang: return "!";
        case Token::Type::Tilde: return "~";
        case Token::Type::Eq: return "==";
        case Token::Type::Ne: return "!=";
        case Token::Type::Lt: return "<";
        case Token::Type::Gt: return ">";
        case Token::Type::Le: return "<=";
        case Token::Type::Ge: return ">=";

        case Token::Type::KwInt: return "int";
        case Token::Type::KwChar: return "char";
        case Token::Type::KwBool: return "bool";
        case Token::Type::KwFloat: return "float";
        case Token::Type::KwVoid: return "void";
        case Token::Type::KwClass: return "class";
        case Token::Type::KwConstructor: return "constructor";
        case Token::Type::KwDestructor: return "destructor";
        case Token::Type::KwIf: return "if";
        case Token::Type::KwElif: return "elif";
        case Token::Type::KwElse: return "else";
        case Token::Type::KwWhile: return "while";
        case Token::Type::KwFor: return "for";
        case Token::Type::KwReturn: return "return";
        case Token::Type::KwBreak: return "break";
        case Token::Type::KwContinue: return "continue";
        case Token::Type::KwPrint: return "print";
        case Token::Type::KwRead: return "read";
        case Token::Type::KwTrue: return "true";
        case Token::Type::KwFalse: return "false";
        case Token::Type::KwAnd: return "and";
        case Token::Type::KwOr: return "or";
        case Token::Type::KwNew: return "new";
        case Token::Type::KwDelete: return "delete";

        case Token::Type::TypeName: return "<type-name>";
    }
    return "<unknown>";
}

inline std::string Token::toString() const {
    return tokenTypeName(type) + " '" + lexeme + "' @" + std::to_string(pos.line) + ":" + std::to_string(pos.column);
}

