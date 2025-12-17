#pragma once
#include <vector>
#include <string>
#include <iostream>


class Poliz {
public:
    struct FunctionInfo {
        std::string name;
        int entryIp;
        int paramCount;

        FunctionInfo(const std::string &n, int ip, int pc)
            : name(n), entryIp(ip), paramCount(pc) {
        }
    };

    enum class Op {
        PUSH_INT,
        PUSH_FLOAT,
        PUSH_CHAR,
        PUSH_BOOL,
        PUSH_STRING,

        LOAD_VAR,
        STORE_VAR,

        ADD,
        SUB,
        MUL,
        DIV,
        MOD,

        NEG,
        NOT,
        BNOT,

        CMP_EQ,
        CMP_NE,
        CMP_LT,
        CMP_LE,
        CMP_GT,
        CMP_GE,

        LOG_AND,
        LOG_OR,

        AND,
        OR,
        XOR,

        SHL,
        SHR,

        JUMP,
        JUMP_IF_FALSE,

        CALL,
        RET_VOID,
        RET_VALUE,

        PRINT,
        READ_INT,
        READ_FLOAT,
        READ_BOOL,
        READ_CHAR,
        READ_STRING,

        NOP,
        HALT,
        LOAD_ELEM,
        STORE_ELEM,
    };

    struct Instr {
        Op op;
        std::optional<int> arg1;
        std::optional<int> arg2;

        Instr(Op o, std::optional<int> arg1 = std::nullopt, std::optional<int> arg2 = std::nullopt)
            : op(o), arg1(arg1), arg2(arg2) {
        }
    };

private:
    std::vector<Instr> code;
    std::vector<std::string> stringPool;
    std::vector<FunctionInfo> functions;

public:
    int emit(Op op,
             std::optional<int> arg1 = std::nullopt,
             std::optional<int> arg2 = std::nullopt) {
        code.emplace_back(op, arg1, arg2);
        return code.size() - 1;
    }

    int emitJump(Op op);

    void patchJump(int instr, int ip);

    int addString(const std::string &s);

    const std::string &getString(int idx) const;

    const Instr &operator[](std::size_t i) const { return code[i]; }
    Instr &operator[](std::size_t i) { return code[i]; }

    std::size_t size() const { return code.size(); }
    bool empty() const { return code.empty(); }

    int currentIp() const { return static_cast<int>(code.size()); }


    void dump(std::ostream &os) const;

    int registerFunction(
        const std::string &name,
        int entryIp,
        int paramCount
    );

    const FunctionInfo &getFunction(int index) const;

    int getFunctionIndex(const std::string &name) const;

    void setFunctionEntry(int index, int entryIp) {
        if (index < 0 || index >= functions.size())
            throw std::runtime_error("Invalid function index");
        functions[index].entryIp = entryIp;
    }
};
