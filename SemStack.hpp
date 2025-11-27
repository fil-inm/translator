#include <stack>
#include <stdexcept>
#include "tokens.hpp"

class SemStack {
private:
    std::stack<Token::Type> st;

    void require(size_t n) {
        if (st.size() < n)
            throw std::runtime_error("semantic stack underflow");
    }

    static bool isNum(Token::Type t) {
        return t == Token::Type::KwInt ||
               t == Token::Type::KwFloat ||
               t == Token::Type::KwChar;
    }

public:
    void push(Token::Type t){
        st.push(t);
    }

    Token::Type pop(){
        require(1);
        Token::Type t = st.top();
        st.pop();
        return t;
    }

    Token::Type popType(){
        return pop();
    }

    Token::Type check_uno(Token::Type op) {
        require(1);
        Token::Type a = pop();

        switch(op){
        case Token::Type::Minus:
            if (isNum(a)) return a;
            throw std::runtime_error("Unary '-' expects numeric");

        case Token::Type::Exclamation:
            if (a == Token::Type::KwBool) return Token::Type::KwBool;
            throw std::runtime_error("Unary '!' expects bool");

        case Token::Type::Tilde:
            if (a == Token::Type::KwInt) return Token::Type::KwInt;
            throw std::runtime_error("Unary '~' expects int");

        case Token::Type::PlusPlus:
        case Token::Type::MinusMinus:
            if (a == Token::Type::KwInt ||
                a == Token::Type::KwFloat ||
                a == Token::Type::KwChar)
                return a;
            throw std::runtime_error("++/-- expects numeric or char");

        default:
            throw std::runtime_error("Invalid unary operator");
        }
    }

    Token::Type check_bin(Token::Type op) {
        require(2);

        Token::Type b = pop();
        Token::Type a = pop();

        if (op == Token::Type::Plus ||
            op == Token::Type::Minus ||
            op == Token::Type::Asterisk ||
            op == Token::Type::Slash ||
            op == Token::Type::Percent) {

            if (a == Token::Type::KwInt && b == Token::Type::KwInt)
                return Token::Type::KwInt;

            if (isNum(a) && isNum(b))
                return Token::Type::KwFloat;

            throw std::runtime_error("Arithmetic type mismatch");
        }
        if (op == Token::Type::Less ||
            op == Token::Type::Greater ||
            op == Token::Type::LessEqual ||
            op == Token::Type::GreaterEqual ||
            op == Token::Type::EqualEqual ||
            op == Token::Type::NotEqual)
            return Token::Type::KwBool;

        if (op == Token::Type::AmpAmp ||
            op == Token::Type::PipePipe) {

            if (a == Token::Type::KwBool && b == Token::Type::KwBool)
                return Token::Type::KwBool;

            throw std::runtime_error("Logical operator requires bool");
        }

        throw std::runtime_error("Unknown binary operator");
    }
};