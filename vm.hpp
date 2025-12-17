#pragma once
#include "poliz.hpp"
#include <vector>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <string>


class InputBuffer {
public:
    explicit InputBuffer(std::istream& in) : in(in) {}

    std::string next() {
        std::string s;
        if (!(in >> s))
            throw std::runtime_error("Input exhausted");
        return s;
    }

private:
    std::istream& in;
};



class VM {
public:
    explicit VM(const Poliz& code, InputBuffer& input);
    void run();

private:
    const Poliz& poliz;
    InputBuffer& input;


    struct Frame {
        int returnIp;
        int savedBase;
        int savedStackSize;

    };

    int base = 0;

    std::vector<Frame> callStack;

    struct Value {
        enum class Kind {
            Int,
            Float,
            Bool,
            Char,
            String
        } kind;

        int   i = 0;
        float f = 0.0f;
        std::string s;

        static Value makeInt(int v) {
            Value x; x.kind = Kind::Int; x.i = v; return x;
        }
        static Value makeFloat(float v) {
            Value x; x.kind = Kind::Float; x.f = v; return x;
        }
        static Value makeBool(bool v) {
            Value x; x.kind = Kind::Bool; x.i = v ? 1 : 0; return x;
        }
        static Value makeChar(char c) {
            Value x; x.kind = Kind::Char; x.i = c; return x;
        }
        static Value makeString(const std::string& str) {
            Value x; x.kind = Kind::String; x.s = str; return x;
        }
    };

    std::vector<Value> stack;


    Value pop();
    void  push(const Value& v);

    Value binaryNumOp(
        const std::function<int(int,int)>&,
        const std::function<float(float,float)>&
    );

    Value binaryCmpOp(const std::function<bool(float,float)>&);

    void printValue(const Value& v);
};