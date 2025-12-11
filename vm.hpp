#pragma once
#include "poliz.hpp"
#include <vector>
#include <iostream>
#include <stdexcept>

class VM {
public:
    explicit VM(const Poliz& code);

    // запустить исполнение с ip = 0
    void run();

private:
    const Poliz& poliz;

    struct Value {
        enum class Kind {
            Int,
            Bool,
            Char,
            String
        } kind;

        int data; // для Int/Bool/Char — значение; для String — индекс в stringPool

        static Value makeInt(int v)    { return { Kind::Int,    v }; }
        static Value makeBool(bool v)  { return { Kind::Bool,   v ? 1 : 0 }; }
        static Value makeChar(char c)  { return { Kind::Char,   static_cast<int>(c) }; }
        static Value makeString(int i) { return { Kind::String, i }; }
    };

    std::vector<Value> stack;

    Value pop();
    void  push(const Value& v);

    // хелперы для арифметики
    Value binaryIntOp(const std::function<int(int,int)>& f);
    void  printValue(const Value& v);
};