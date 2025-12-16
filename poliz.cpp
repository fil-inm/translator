#include "poliz.hpp"
#include <stdexcept>

// ============================
// Генерация инструкций
// ============================

int Poliz::emit(Op op, int arg1, int arg2) {
    code.emplace_back(op, arg1, arg2);
    return code.size() - 1;
}

int Poliz::emitJump(Op op) {
    return emit(op, -1);
}

void Poliz::patchJump(int instrIndex, int targetIp) {
    if (instrIndex < 0 || instrIndex >= static_cast<int>(code.size())) {
        throw std::runtime_error("Poliz::patchJump: invalid index");
    }
    code[instrIndex].arg1 = targetIp;
}

// ============================
// Строковый пул
// ============================

int Poliz::addString(const std::string &s) {
    stringPool.push_back(s);
    return static_cast<int>(stringPool.size()) - 1;
}

const std::string &Poliz::getString(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(stringPool.size())) {
        throw std::runtime_error("Poliz::getString: invalid index");
    }
    return stringPool[idx];
}

// ============================
// Dump
// ============================

static const char *opName(Poliz::Op op) {
    using Op = Poliz::Op;
    switch (op) {
        case Op::PUSH_INT: return "PUSH_INT";
        case Op::PUSH_FLOAT: return "PUSH_FLOAT";
        case Op::PUSH_CHAR: return "PUSH_CHAR";
        case Op::PUSH_BOOL: return "PUSH_BOOL";
        case Op::PUSH_STRING: return "PUSH_STRING";

        case Op::LOAD_VAR: return "LOAD_VAR";
        case Op::STORE_VAR: return "STORE_VAR";

        case Op::ADD: return "ADD";
        case Op::SUB: return "SUB";
        case Op::MUL: return "MUL";
        case Op::DIV: return "DIV";
        case Op::MOD: return "MOD";

        case Op::NEG: return "NEG";
        case Op::NOT: return "NOT";
        case Op::BNOT: return "BNOT";

        case Op::CMP_EQ: return "CMP_EQ";
        case Op::CMP_NE: return "CMP_NE";
        case Op::CMP_LT: return "CMP_LT";
        case Op::CMP_LE: return "CMP_LE";
        case Op::CMP_GT: return "CMP_GT";
        case Op::CMP_GE: return "CMP_GE";

        case Op::LOG_AND: return "LOG_AND";
        case Op::LOG_OR: return "LOG_OR";

        case Op::JUMP: return "JUMP";
        case Op::JUMP_IF_FALSE: return "JUMP_IF_FALSE";

        case Op::CALL: return "CALL";
        case Op::RET: return "RET";

        case Op::PRINT: return "PRINT";
        case Op::READ: return "READ";

        case Op::NOP: return "NOP";
        case Op::HALT: return "HALT";
        case Op::AND: return "AND";
        case Op::OR: return "OR";
        case Op::XOR: return "XOR";
        case Op::SHL: return "SHL";
        case Op::SHR: return "SHR";
        default: return "UNKNOWN";
    }
}

void Poliz::dump(std::ostream &os) const {
    os << "=== POLIZ dump ===\n";
    for (std::size_t i = 0; i < code.size(); ++i) {
        const auto &ins = code[i];
        os << i << ":\t" << opName(ins.op);

        if (ins.arg1 != 0 || ins.arg2 != 0) {
            os << " " << ins.arg1;
            if (ins.arg2 != 0)
                os << ", " << ins.arg2;
        }
        os << "\n";
    }

    if (!stringPool.empty()) {
        os << "--- String pool ---\n";
        for (std::size_t i = 0; i < stringPool.size(); ++i) {
            os << i << ": \"" << stringPool[i] << "\"\n";
        }
    }

    os << "===================\n";
}
