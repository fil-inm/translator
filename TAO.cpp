#include "TAO.hpp"
#include "tokens.hpp"

void SemStack::push(Token::Type &t, const string& val){
    st.push({t, val});
}

SItem SemStack::pop(){
    auto x = st.top();
    st.pop();
    return x;
}

Token::Type SemStack::check_uno(){
    Token::Type a = pop().kind;
    Token::Type op = pop().kind;

    if (op == Token::Type::Minus) {
        if (a == Token::Type::KwInt || a == Token::Type::KwFloat) return a;
        throw runtime_error("Unary - requires numeric operand");
    }

    if (op == Token::Type::Exclamation) {
        if (a == Token::Type::KwBool && a == Token::Type::KwInt) return Token::Type::KwBool;
        throw runtime_error("Unary ! requires bool operand");
    }

    if (op == Token::Type::Tilde) {
        if (a == Token::Type::KwInt) return Token::Type::KwInt;
        throw runtime_error("Unary ~ requires int operand");
    }

    if(op == Token::Type::PlusPlus || op == Token::Type::MinusMinus) {
        if (a == Token::Type::KwInt || a == Token::Type::KwFloat || a == Token::Type::KwChar) return a;
        throw runtime_error("Unary pref inkrement / decrement requires int operand");
    }

    throw runtime_error("Something went wrong");
}

Token::Type SemStack::check_bin(){
    Token::Type a = pop().kind;
    Token::Type op = pop().kind;
    Token::Type b = pop().kind;

    if (op == Token::Type::Plus || op == Token::Type::Minus || op == Token::Type::Asterisk || op == Token::Type::Slash) {
        if(a == Token::Type::KwInt && b == Token::Type::KwInt) return Token::Type::KwInt;
        if ((a == Token::Type::KwInt || a == Token::Type::KwFloat) && (b == Token::Type::KwInt || b == Token::Type::KwFloat)) return Token::Type::KwFloat;
        throw runtime_error("Type error in binary arithmetic expression");
    }

    if (op == Token::Type::Less || op == Token::Type::Greater || op == Token::Type::LessEqual || op == Token::Type::GreaterEqual ||
        op == Token::Type::EqualEqual || op == Token::Type::NotEqual){ return Token::Type::KwBool;}

    if (op == Token::Type::AmpAmp || op == Token::Type::PipePipe) {
        if (a == Token::Type::KwBool && b == Token::Type::KwBool) return Token::Type::KwBool;
        throw runtime_error("Logical operation requires boolean operands");
    }

    throw runtime_error("Something went wrong");
}