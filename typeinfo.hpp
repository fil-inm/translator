#pragma once
#include <string>
#include <optional>
#include "tokens.hpp"

struct TypeInfo {
    Token::Type baseType;
    bool isArray = false;
    int arraySize = -1;
    std::optional<Token::Type> elementType;

    TypeInfo() : baseType(Token::Type::KwVoid) {}
    TypeInfo(Token::Type t) : baseType(t) {}

    static TypeInfo makeArray(const TypeInfo& elem, int size = -1) {
        TypeInfo t;
        t.isArray = true;
        t.arraySize = size;
        t.elementType = elem.baseType;
        t.baseType = elem.baseType;
        return t;
    }

    bool isVoid() const {
        return baseType == Token::Type::KwVoid && !isArray;
    }

    bool isNumeric() const {
        return baseType == Token::Type::KwInt ||
               baseType == Token::Type::KwFloat ||
               baseType == Token::Type::KwChar;
    }

    bool isIntegral() const {
        return baseType == Token::Type::KwInt ||
               baseType == Token::Type::KwChar ||
               baseType == Token::Type::KwBool;
    }

    bool isBool() const {
        return baseType == Token::Type::KwBool;
    }

    bool isChar() const {
        return baseType == Token::Type::KwChar;
    }

    bool operator==(const TypeInfo& o) const {
        if (isArray != o.isArray)
            return false;
        if (isArray)
            return baseType == o.baseType &&
                   arraySize == o.arraySize;
        return baseType == o.baseType;
    }

    bool operator!=(const TypeInfo& o) const {
        return !(*this == o);
    }

    std::string toString() const {
        std::string s;
        switch (baseType) {
            case Token::Type::KwInt:   s = "int"; break;
            case Token::Type::KwFloat: s = "float"; break;
            case Token::Type::KwBool:  s = "bool"; break;
            case Token::Type::KwChar:  s = "char"; break;
            case Token::Type::KwVoid:  s = "void"; break;
            default:                   s = "<unknown>";
        }

        if (isArray) {
            s += "[";
            if (arraySize >= 0)
                s += std::to_string(arraySize);
            s += "]";
        }

        return s;
    }
};