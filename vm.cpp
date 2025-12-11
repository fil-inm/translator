#include "vm.hpp"

// ==========================
// Конструктор
// ==========================

VM::VM(const Poliz& code)
    : poliz(code) {
}

// ==========================
// Стековые операции
// ==========================

VM::Value VM::pop() {
    if (stack.empty()) {
        throw std::runtime_error("VM stack underflow");
    }
    Value v = stack.back();
    stack.pop_back();
    return v;
}

void VM::push(const Value& v) {
    stack.push_back(v);
}

// Арифметика над целыми
VM::Value VM::binaryIntOp(const std::function<int(int,int)>& f) {
    Value b = pop();
    Value a = pop();

    if (a.kind != Value::Kind::Int || b.kind != Value::Kind::Int) {
        throw std::runtime_error("VM: arithmetic only implemented for Int right now");
    }

    int res = f(a.data, b.data);
    return Value::makeInt(res);
}

// Печать
void VM::printValue(const Value& v) {
    switch (v.kind) {
        case Value::Kind::Int:
            std::cout << v.data;
            break;

        case Value::Kind::Bool:
            std::cout << (v.data ? "true" : "false");
            break;

        case Value::Kind::Char:
            std::cout << static_cast<char>(v.data);
            break;

        case Value::Kind::String: {
            const std::string& s = poliz.getString(v.data);
            std::cout << s;
            break;
        }

        default:
            std::cout << "<unknown>";
            break;
    }
}

// ==========================
// Исполнение
// ==========================

void VM::run() {
    int ip = 0;
    const int n = static_cast<int>(poliz.size());

    while (ip < n) {
        const auto& ins = poliz[ip];

        switch (ins.op) {
            // ----- PUSH'и -----
            case Poliz::Op::PUSH_INT:
                push(Value::makeInt(ins.arg1));
                ++ip;
                break;

            case Poliz::Op::PUSH_BOOL:
                push(Value::makeBool(ins.arg1 != 0));
                ++ip;
                break;

            case Poliz::Op::PUSH_CHAR:
                push(Value::makeChar(static_cast<char>(ins.arg1)));
                ++ip;
                break;

            case Poliz::Op::PUSH_STRING:
                push(Value::makeString(ins.arg1));
                ++ip;
                break;

            // ----- Арифметика -----
            case Poliz::Op::ADD: {
                Value r = binaryIntOp([](int a, int b){ return a + b; });
                push(r);
                ++ip;
                break;
            }

            case Poliz::Op::SUB: {
                Value r = binaryIntOp([](int a, int b){ return a - b; });
                push(r);
                ++ip;
                break;
            }

            case Poliz::Op::MUL: {
                Value r = binaryIntOp([](int a, int b){ return a * b; });
                push(r);
                ++ip;
                break;
            }

            case Poliz::Op::DIV: {
                Value r = binaryIntOp([](int a, int b){
                    if (b == 0) throw std::runtime_error("VM: division by zero");
                    return a / b;
                });
                push(r);
                ++ip;
                break;
            }

            case Poliz::Op::MOD: {
                Value r = binaryIntOp([](int a, int b){
                    if (b == 0) throw std::runtime_error("VM: modulo by zero");
                    return a % b;
                });
                push(r);
                ++ip;
                break;
            }

            // ----- Унарные -----
            case Poliz::Op::NEG: {
                Value v = pop();
                if (v.kind != Value::Kind::Int)
                    throw std::runtime_error("VM: NEG only for Int");
                v.data = -v.data;
                push(v);
                ++ip;
                break;
            }

            case Poliz::Op::NOT: {
                Value v = pop();
                // трактуем всё как "логическое не ноль"
                int iv = (v.kind == Value::Kind::Bool || v.kind == Value::Kind::Int)
                         ? v.data : 0;
                push(Value::makeBool(iv == 0));
                ++ip;
                break;
            }

            case Poliz::Op::BNOT: {
                Value v = pop();
                if (v.kind != Value::Kind::Int)
                    throw std::runtime_error("VM: BNOT only for Int");
                v.data = ~v.data;
                push(v);
                ++ip;
                break;
            }

            // ----- PRINT -----
            case Poliz::Op::PRINT: {
                Value v = pop();
                printValue(v);
                std::cout << "\n";
                ++ip;
                break;
            }

            // ----- HALT -----
            case Poliz::Op::HALT:
                return;

            // Остальные пока не реализованы
            default:
                throw std::runtime_error("VM: opcode not implemented yet");
        }
    }
}