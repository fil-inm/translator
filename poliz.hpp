#pragma once
#include <vector>
#include <string>
#include <iostream>

// Стековый байткод для твоего языка
class Poliz {
public:
    // ---------------------------
    // Опкоды виртуальной машины
    // ---------------------------
    enum class Op {
        // Литералы / константы
        PUSH_INT,      // arg1: значение
        PUSH_FLOAT,    // arg1: индекс в пуле констант/пока можно не использовать
        PUSH_CHAR,     // arg1: символ (char)
        PUSH_BOOL,     // arg1: 0 или 1
        PUSH_STRING,   // arg1: индекс в stringPool

        // Работа с переменными
        LOAD_VAR,      // arg1: индекс переменной
        STORE_VAR,     // arg1: индекс переменной

        // Арифметика
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,

        // Унарные
        NEG,           // -x
        NOT,           // !x
        BNOT,          // ~x

        // Сравнение
        CMP_EQ,
        CMP_NE,
        CMP_LT,
        CMP_LE,
        CMP_GT,
        CMP_GE,

        // Логика (можно реализовать и через CMP + bool, но пусть будут)
        LOG_AND,
        LOG_OR,

        // Переходы
        JUMP,          // arg1: адрес
        JUMP_IF_FALSE, // arg1: адрес

        // Функции
        CALL,          // arg1: индекс функции (или адрес)
        RET,           // без аргументов

        // Ввод/вывод
        PRINT,         // печатает вершину стека
        READ,          // читает в вершину стека / позже уточним

        // Служебное
        NOP,
        HALT
    };

    // Одна инструкция байткода
    struct Instr {
        Op   op;
        int  arg1;   // первый аргумент (индекс, смещение, значение)
        int  arg2;   // запасной аргумент, на будущее (можно не использовать)

        Instr(Op op_, int a1 = 0, int a2 = 0)
            : op(op_), arg1(a1), arg2(a2) {}
    };

private:
    std::vector<Instr> code;
    std::vector<std::string> stringPool;

public:
    // ---------------------------
    // Добавление инструкций
    // ---------------------------

    // Просто добавить инструкцию, вернуть её индекс
    int emit(Op op, int arg1 = 0, int arg2 = 0);

    // Зарезервировать прыжок с ещё неизвестной целью.
    // Возвращает индекс инструкции, чтобы потом допатчить.
    int emitJump(Op op);

    // Дописать адрес прыжка в уже сгенерированную инструкцию
    void patchJump(int instrIndex, int targetIp);

    // ---------------------------
    // Работа со строковыми литералами
    // ---------------------------

    // Положить строку в пул и вернуть её индекс
    int addString(const std::string& s);

    const std::string& getString(int idx) const;

    // ---------------------------
    // Доступ к коду
    // ---------------------------

    const Instr& operator[](std::size_t i) const { return code[i]; }
    Instr&       operator[](std::size_t i)       { return code[i]; }

    std::size_t size() const { return code.size(); }
    bool        empty() const { return code.empty(); }

    // Текущая "адрес" следующей инструкции
    int currentIp() const { return static_cast<int>(code.size()); }

    // ---------------------------
    // Отладочный вывод
    // ---------------------------
    void dump(std::ostream& os) const;
};