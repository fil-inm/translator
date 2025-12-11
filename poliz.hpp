#pragma once
#include <vector>
#include <string>
#include <iostream>

class Poliz {
public:
    enum class Op {
        // Литералы / константы
        PUSH_INT,
        PUSH_FLOAT,
        PUSH_CHAR,
        PUSH_BOOL,
        PUSH_STRING,

        // Переменные (позже прикрутим)
        LOAD_VAR,
        STORE_VAR,

        // Арифметика
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,

        // Унарные
        NEG,    // -x
        NOT,    // !x
        BNOT,   // ~x

        // Сравнение
        CMP_EQ,
        CMP_NE,
        CMP_LT,
        CMP_LE,
        CMP_GT,
        CMP_GE,

        // Логика
        LOG_AND,
        LOG_OR,

        // Переходы (понадобятся позже для if/while/for)
        JUMP,
        JUMP_IF_FALSE,

        // Функции (тоже позже)
        CALL,
        RET,

        // Ввод/вывод
        PRINT,
        READ,

        // Служебное
        NOP,
        HALT
    };

    struct Instr {
        Op  op;
        int arg1;
        int arg2;

        Instr(Op o, int a1 = 0, int a2 = 0)
            : op(o), arg1(a1), arg2(a2) {}
    };

private:
    std::vector<Instr>       code;
    std::vector<std::string> stringPool;

public:
    // Добавление инструкций
    int emit(Op op, int arg1 = 0, int arg2 = 0);
    int emitJump(Op op);                    // JUMP / JUMP_IF_FALSE с placeholder
    void patchJump(int instrIndex, int targetIp);

    // Строковый пул
    int addString(const std::string& s);
    const std::string& getString(int idx) const;

    // Доступ к коду
    const Instr& operator[](std::size_t i) const { return code[i]; }
    Instr&       operator[](std::size_t i)       { return code[i]; }

    std::size_t size()  const { return code.size(); }
    bool        empty() const { return code.empty(); }

    int currentIp() const { return static_cast<int>(code.size()); }

    // Отладочный вывод
    void dump(std::ostream& os) const;
};