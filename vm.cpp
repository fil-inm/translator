#include "vm.hpp"
#include <sstream>



VM::VM(const Poliz &code, InputBuffer &in)
    : poliz(code), input(in) {
}


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


void VM::run() {
    int ip = 0;

    while (ip < (int) poliz.size()) {
        const auto &ins = poliz[ip];

        switch (ins.op) {
            case Poliz::Op::PUSH_INT:
                push(Value::makeInt(*ins.arg1));
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
                push(Value::makeChar(static_cast<char>(*ins.arg1)));
                ++ip;
                break;

            case Poliz::Op::PUSH_STRING:
                push(Value::makeString(poliz.getString(*ins.arg1)));
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
                int idx = base + *ins.arg1;
                if (idx >= (int) stack.size())
                    stack.resize(idx + 1);
                stack[idx] = v;
                ++ip;
                break;
            }

            case Poliz::Op::LOAD_VAR: {
                int idx = base + *ins.arg1;
                if (idx < 0 || idx >= (int) stack.size())
                    throw std::runtime_error("LOAD_VAR out of range");
                push(stack[idx]);
                ++ip;
                break;
            }

            case Poliz::Op::LOAD_ELEM: {
                Value idx = pop();
                int baseSlot = base + *ins.arg1;

                if (idx.kind != Value::Kind::Int)
                    throw std::runtime_error("LOAD_ELEM: index must be int");

                int addr = baseSlot + idx.i;

                if (addr < 0 || addr >= stack.size())
                    throw std::runtime_error("LOAD_ELEM: out of range");

                push(stack[addr]);
                ++ip;
                break;
            }

            case Poliz::Op::STORE_ELEM: {
                Value value = pop();
                Value idx   = pop();

                if (idx.kind != Value::Kind::Int)
                    throw std::runtime_error("STORE_ELEM: index must be int");

                int baseSlot = base + *ins.arg1;
                int addr = baseSlot + idx.i;

                if (addr < 0)
                    throw std::runtime_error("STORE_ELEM: negative index");

                if (addr >= stack.size())
                    stack.resize(addr + 1);

                stack[addr] = value;
                ++ip;
                break;
            }

            case Poliz::Op::CALL: {
                const auto& f = poliz.getFunction(*ins.arg1);

                int argBase = stack.size() - f.paramCount;
                if (argBase < 0)
                    throw std::runtime_error("CALL: not enough args");

                callStack.push_back({
                    ip + 1,
                    base,
                    argBase
                });

                base = argBase;
                ip = f.entryIp;
                break;
            }

            case Poliz::Op::RET_VALUE: {
                Value ret = pop();

                if (callStack.empty())
                    throw std::runtime_error("RET_VALUE with empty call stack");

                Frame fr = callStack.back();
                callStack.pop_back();

                stack.resize(fr.savedStackSize);
                base = fr.savedBase;
                ip = fr.returnIp;

                push(ret);
                break;
            }

            case Poliz::Op::RET_VOID: {
                if (callStack.empty()) {
                    ip = poliz.size();
                    break;
                }

                Frame fr = callStack.back();
                callStack.pop_back();

                stack.resize(fr.savedStackSize);
                base = fr.savedBase;
                ip = fr.returnIp;
                break;
            }

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

            case Poliz::Op::JUMP:
                ip = *ins.arg1;
                break;

            case Poliz::Op::JUMP_IF_FALSE:
                if (pop().i == 0) ip = *ins.arg1;
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
