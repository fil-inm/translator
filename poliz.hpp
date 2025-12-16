#pragma once
#include <vector>
#include <string>
#include <iostream>



class Poliz {
public:
    enum class Op {
        // ===== Константы =====
        PUSH_INT, // arg1 = int
        PUSH_FLOAT, // (пока не используется)
        PUSH_CHAR, // arg1 = char
        PUSH_BOOL, // arg1 = 0/1
        PUSH_STRING, // arg1 = string index

        // ===== Переменные =====
        LOAD_VAR, // arg1 = slot
        STORE_VAR, // arg1 = slot

        // ===== Арифметика =====
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,

        // ===== Унарные =====
        NEG,
        NOT,
        BNOT,

        // ===== Сравнения =====
        CMP_EQ,
        CMP_NE,
        CMP_LT,
        CMP_LE,
        CMP_GT,
        CMP_GE,

        // ===== Логика =====
        LOG_AND,
        LOG_OR,

        // ===== Битовые операции =====
        AND, // &
        OR, // |
        XOR, // ^

        // ===== Сдвиги =====
        SHL, // <<
        SHR, // >>

        // ===== Управление потоком =====
        JUMP, // arg1 = target ip
        JUMP_IF_FALSE, // arg1 = target ip

        // ===== Функции =====
        CALL, // arg1 = func index
        RET,

        // ===== Ввод/вывод =====
        PRINT,
        READ,
    READ_INT,
    READ_FLOAT,
    READ_BOOL,
    READ_CHAR,
        READ_STRING,   // ← ДОБАВИТЬ

        // ===== Служебные =====
        NOP,
        HALT
    };

    struct Instr {
        Op op;
        int arg1;
        int arg2;

        Instr(Op o, int a1 = 0, int a2 = 0)
            : op(o), arg1(a1), arg2(a2) {
        }
    };

private:
    std::vector<Instr> code;
    std::vector<std::string> stringPool;

public:
    // ===== Генерация =====
    int emit(Op op, int arg1 = 0, int arg2 = 0);

    int emitJump(Op op); // JUMP / JUMP_IF_FALSE
    void patchJump(int instr, int ip);

    // ===== Строки =====
    int addString(const std::string &s);

    const std::string &getString(int idx) const;

    // ===== Доступ =====
    const Instr &operator[](std::size_t i) const { return code[i]; }
    Instr &operator[](std::size_t i) { return code[i]; }

    std::size_t size() const { return code.size(); }
    bool empty() const { return code.empty(); }

    int currentIp() const { return static_cast<int>(code.size()); }

    // ===== Отладка =====
    void dump(std::ostream &os) const;
};
