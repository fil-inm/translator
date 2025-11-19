#include "TAO.hpp"

void SemStack::pushType(const string &t){
    st.push({SItem::TYPE, t});
}

void SemStack::pushOp(const string &op){
    st.push({SItem::OP, op});
}

SItem SemStack::pop(){
    auto x = st.top();
    st.pop();
    return x;
}

SItem SemStack::top() const{
    return st.top();
}

string check_bin(const string &a, const string &op, const string &b) {
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        if (a == "int" && b == "int") return "int";
        if ((a == "float" && b == "int") || (a == "int" && b == "float")) return "float";
        if (a == "float" && b == "float") return "float";
        throw string("Type error in binary arithmetic expression");
    }

    if (op == "<" || op == ">" || op == "<=" || op == ">=" ||
        op == "==" || op == "!="){ return "bool";}

    if (op == "&&" || op == "||") {
        if (a == "bool" && b == "bool") return "bool";
        throw string("Logical operation requires boolean operands");
    }

    throw string("Unknown binary operator: " + op);
}

string check_uno(const string &op, const string &a) {
    if (op == "+" || op == "-") {
        if (a == "int" || a == "float") return a;
        throw string("Unary +/- requires numeric operand");
    }

    if (op == "!") {
        if (a == "bool") return "bool";
        throw string("Unary ! requires bool operand");
    }

    if (op == "~") {
        if (a == "int") return "int";
        throw string("Unary ~ requires int operand");
    }

    throw string("Unknown unary operator: " + op);
}