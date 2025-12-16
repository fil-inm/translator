#include "vm.hpp"
#include <sstream>

// ==========================
// Конструктор
// ==========================

VM::VM(const Poliz &code, InputBuffer &in)
    : poliz(code), input(in) {
}

// ==========================
// Стек
// ==========================

VM::Value VM::pop() {
    if (stack.empty())
        throw std::runtime_error("VM: stack underflow");
    Value v = stack.back();
    stack.pop_back();
    return v;
}

void VM::push(const Value &v) {
    stack.push_back(v);
}

// ==========================
// Арифметика
// ==========================

VM::Value VM::binaryNumOp(
    const std::function<int(int, int)> &intOp,
    const std::function<float(float, float)> &floatOp
) {
    Value b = pop();
    Value a = pop();

    if (a.kind == Value::Kind::Int && b.kind == Value::Kind::Int)
        return Value::makeInt(intOp(a.i, b.i));

    float af = (a.kind == Value::Kind::Float) ? a.f : a.i;
    float bf = (b.kind == Value::Kind::Float) ? b.f : b.i;

    return Value::makeFloat(floatOp(af, bf));
}

VM::Value VM::binaryCmpOp(const std::function<bool(float, float)> &f) {
    Value b = pop();
    Value a = pop();

    float af = (a.kind == Value::Kind::Float) ? a.f : a.i;
    float bf = (b.kind == Value::Kind::Float) ? b.f : b.i;

    return Value::makeBool(f(af, bf));
}

// ==========================
// Печать
// ==========================

void VM::printValue(const Value &v) {
    switch (v.kind) {
        case Value::Kind::Int: std::cout << v.i;
            break;
        case Value::Kind::Float: std::cout << v.f;
            break;
        case Value::Kind::Bool: std::cout << (v.i ? "true" : "false");
            break;
        case Value::Kind::Char: std::cout << static_cast<char>(v.i);
            break;
        case Value::Kind::String: std::cout << v.s;
            break;
    }
}

// ==========================
// Исполнение
// ==========================

void VM::run() {
    int ip = 0;
    locals.clear();

    while (ip < (int) poliz.size()) {
        const auto &ins = poliz[ip];

        switch (ins.op) {
            // ===== PUSH =====
            case Poliz::Op::PUSH_INT:
                push(Value::makeInt(ins.arg1));
                ++ip;
                break;

            case Poliz::Op::PUSH_FLOAT: {
                float v;
                std::memcpy(&v, &ins.arg1, sizeof(float));
                push(Value::makeFloat(v));
                ++ip;
                break;
            }

            case Poliz::Op::PUSH_BOOL:
                push(Value::makeBool(ins.arg1 != 0));
                ++ip;
                break;

            case Poliz::Op::PUSH_CHAR:
                push(Value::makeChar(static_cast<char>(ins.arg1)));
                ++ip;
                break;

            case Poliz::Op::PUSH_STRING:
                push(Value::makeString(poliz.getString(ins.arg1)));
                ++ip;
                break;

            // ===== Переменные =====
            case Poliz::Op::LOAD_VAR:
                push(locals.at(ins.arg1));
                ++ip;
                break;

            case Poliz::Op::NOT: {
                Value a = pop();

                if (a.kind != Value::Kind::Bool)
                    throw std::runtime_error("VM: NOT only for Bool");

                push(Value::makeBool(!a.i));
                ++ip;
                break;
            }

            case Poliz::Op::NEG: {
                Value a = pop();

                if (a.kind == Value::Kind::Int)
                    push(Value::makeInt(-a.i));
                else if (a.kind == Value::Kind::Float)
                    push(Value::makeFloat(-a.f));
                else
                    throw std::runtime_error("VM: NEG only for numeric types");

                ++ip;
                break;
            }

            case Poliz::Op::BNOT: {
                Value a = pop();

                if (a.kind != Value::Kind::Int)
                    throw std::runtime_error("VM: BNOT only for Int");

                push(Value::makeInt(~a.i));
                ++ip;
                break;
            }

            case Poliz::Op::STORE_VAR: {
                Value v = pop();
                if (ins.arg1 >= (int) locals.size())
                    locals.resize(ins.arg1 + 1);
                locals[ins.arg1] = v;
                ++ip;
                break;
            }

            // ===== Арифметика =====
            case Poliz::Op::ADD:
                push(binaryNumOp(
                    [](int a, int b) { return a + b; },
                    [](float a, float b) { return a + b; }
                ));
                ++ip;
                break;

            case Poliz::Op::SUB:
                push(binaryNumOp(
                    [](int a, int b) { return a - b; },
                    [](float a, float b) { return a - b; }
                ));
                ++ip;
                break;

            case Poliz::Op::MUL:
                push(binaryNumOp(
                    [](int a, int b) { return a * b; },
                    [](float a, float b) { return a * b; }
                ));
                ++ip;
                break;

            case Poliz::Op::DIV:
                push(binaryNumOp(
                    [](int a, int b) {
                        if (b == 0) throw;
                        return a / b;
                    },
                    [](float a, float b) {
                        if (b == 0) throw;
                        return a / b;
                    }
                ));
                ++ip;
                break;

            // ===== Битовые =====
            case Poliz::Op::AND: {
                Value b = pop();
                Value a = pop();
                if (a.kind != Value::Kind::Int || b.kind != Value::Kind::Int)
                    throw std::runtime_error("VM: AND only for Int");
                push(Value::makeInt(a.i & b.i));
                ++ip;
                break;
            }

            case Poliz::Op::OR: {
                Value b = pop();
                Value a = pop();
                if (a.kind != Value::Kind::Int || b.kind != Value::Kind::Int)
                    throw std::runtime_error("VM: OR only for Int");
                push(Value::makeInt(a.i | b.i));
                ++ip;
                break;
            }

            case Poliz::Op::XOR: {
                Value b = pop();
                Value a = pop();
                if (a.kind != Value::Kind::Int || b.kind != Value::Kind::Int)
                    throw std::runtime_error("VM: XOR only for Int");
                push(Value::makeInt(a.i ^ b.i));
                ++ip;
                break;
            }


            case Poliz::Op::SHL: {
                Value b = pop();
                Value a = pop();
                if (a.kind != Value::Kind::Int || b.kind != Value::Kind::Int)
                    throw std::runtime_error("VM: SHL only for Int");
                push(Value::makeInt(a.i << b.i));
                ++ip;
                break;
            }

            case Poliz::Op::SHR: {
                Value b = pop();
                Value a = pop();
                if (a.kind != Value::Kind::Int || b.kind != Value::Kind::Int)
                    throw std::runtime_error("VM: SHR only for Int");
                push(Value::makeInt(a.i >> b.i));
                ++ip;
                break;
            }


            // ===== Сравнения =====
            case Poliz::Op::CMP_EQ:
                push(binaryCmpOp([](float a, float b) { return a == b; }));
                ++ip;
                break;
            case Poliz::Op::CMP_NE:
                push(binaryCmpOp([](float a, float b) { return a != b; }));
                ++ip;
                break;
            case Poliz::Op::CMP_LT:
                push(binaryCmpOp([](float a, float b) { return a < b; }));
                ++ip;
                break;
            case Poliz::Op::CMP_LE:
                push(binaryCmpOp([](float a, float b) { return a <= b; }));
                ++ip;
                break;
            case Poliz::Op::CMP_GT:
                push(binaryCmpOp([](float a, float b) { return a > b; }));
                ++ip;
                break;
            case Poliz::Op::CMP_GE:
                push(binaryCmpOp([](float a, float b) { return a >= b; }));
                ++ip;
                break;

            // ===== Логика =====
            case Poliz::Op::LOG_AND: {
                Value b = pop();
                Value a = pop();
                push(Value::makeBool(a.i && b.i));
                ++ip;
                break;
            }

            case Poliz::Op::LOG_OR: {
                Value b = pop();
                Value a = pop();
                push(Value::makeBool(a.i || b.i));
                ++ip;
                break;
            }

            // ===== Переходы =====
            case Poliz::Op::JUMP:
                ip = ins.arg1;
                break;

            case Poliz::Op::JUMP_IF_FALSE:
                if (pop().i == 0) ip = ins.arg1;
                else ++ip;
                break;

            case Poliz::Op::READ_INT: {
                std::string s = input.next();
                try {
                    int v = std::stoi(s);
                    push(Value::makeInt(v));
                } catch (...) {
                    throw std::runtime_error("Invalid int input: " + s);
                }
                ++ip;
                break;
            }

            case Poliz::Op::READ_FLOAT: {
                std::string s = input.next();
                try {
                    float v = std::stof(s);
                    push(Value::makeFloat(v));
                } catch (...) {
                    throw std::runtime_error("Invalid float input: " + s);
                }
                ++ip;
                break;
            }

            case Poliz::Op::READ_BOOL: {
                std::string s = input.next();
                if (s == "true")
                    push(Value::makeBool(true));
                else if (s == "false")
                    push(Value::makeBool(false));
                else
                    throw std::runtime_error("Invalid bool input: " + s);
                ++ip;
                break;
            }

            case Poliz::Op::READ_CHAR: {
                std::string s = input.next();
                if (s.size() != 1)
                    throw std::runtime_error("Invalid char input: " + s);
                push(Value::makeChar(s[0]));
                ++ip;
                break;
            }

            case Poliz::Op::READ_STRING: {
                push(Value::makeString(input.next()));
                ++ip;
                break;
            }

            // ===== I/O =====
            case Poliz::Op::PRINT:
                printValue(pop());
                std::cout << "\n";
                ++ip;
                break;

            case Poliz::Op::MOD: {
                Value b = pop();
                Value a = pop();

                if (a.kind != Value::Kind::Int || b.kind != Value::Kind::Int)
                    throw std::runtime_error("VM: MOD only for Int");

                if (b.i == 0)
                    throw std::runtime_error("VM: modulo by zero");

                push(Value::makeInt(a.i % b.i));
                ++ip;
                break;
            }

            // ===== Завершение =====
            case Poliz::Op::HALT:
                return;

            default: {
                std::ostringstream oss;
                oss << "VM: opcode not implemented: "
                        << static_cast<int>(ins.op);
                throw std::runtime_error(oss.str());
            }
        }
    }
}
