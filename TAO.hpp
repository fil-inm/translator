#include <stack>
#include <stdexcept>
#include "tokens.hpp"

struct SItem{
    Token::Type kind;
    std::string value;
};

class SemStack {
private:
    std::stack<SItem> st;

    void require(int n){
        if(st.size()<n)
            throw std::runtime_error("sem stack underflow");
    }

public:
    void push(Token::Type t, const std::string& v=""){
        st.push({t,v});
    }

    SItem pop(){
        require(1);
        auto x=st.top();
        st.pop();
        return x;
    }

    Token::Type check_unary(){
        require(2);

        auto a = pop();
        auto op = pop();

        switch(op.kind){
        case Token::Type::Minus:
            if(a.kind==Token::Type::KwInt || a.kind==Token::Type::KwFloat)
                return a.kind;
            break;

        case Token::Type::Exclamation:
            if(a.kind==Token::Type::KwBool)
                return Token::Type::KwBool;
            break;

        case Token::Type::Tilde:
            if(a.kind==Token::Type::KwInt)
                return Token::Type::KwInt;
            break;

        case Token::Type::PlusPlus:
        case Token::Type::MinusMinus:
            if(a.kind==Token::Type::KwInt || a.kind==Token::Type::KwFloat || a.kind==Token::Type::KwChar)
                return a.kind;
            break;
        default:
            throw std::runtime_error("invalid unary operation");
        }
        throw std::runtime_error("invalid unary operation");
    }

    Token::Type check_bin(){
        require(3);

        auto b = pop();
        auto op = pop();
        auto a = pop();

        if(op.kind==Token::Type::Plus ||
           op.kind==Token::Type::Minus ||
           op.kind==Token::Type::Asterisk||
           op.kind==Token::Type::Slash)
        {
            if(a.kind==Token::Type::KwInt && b.kind==Token::Type::KwInt)
                return Token::Type::KwInt;

            if((a.kind==Token::Type::KwInt||a.kind==Token::Type::KwFloat)
            && (b.kind==Token::Type::KwInt||b.kind==Token::Type::KwFloat))
                return Token::Type::KwFloat;

            throw std::runtime_error("bad arithmetic types");
        }

        if(op.kind==Token::Type::Less ||
           op.kind==Token::Type::Greater ||
           op.kind==Token::Type::LessEqual ||
           op.kind==Token::Type::GreaterEqual ||
           op.kind==Token::Type::EqualEqual ||
           op.kind==Token::Type::NotEqual)
            return Token::Type::KwBool;

        if(op.kind==Token::Type::AmpAmp||
           op.kind==Token::Type::PipePipe)
        {
            if(a.kind==Token::Type::KwBool && b.kind==Token::Type::KwBool)
                return Token::Type::KwBool;
            throw std::runtime_error("logical expects bool");
        }
        throw std::runtime_error("unknown binary operation");
    }
};
