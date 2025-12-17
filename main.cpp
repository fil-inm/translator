#include "lexer.hpp"
#include "semanter.hpp"
#include "parser.hpp"
#include "poliz.hpp"
#include "vm.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

int main() {
    const std::string keywordsFile = "keywords.txt";

    std::vector<std::string> testFiles = {
        "tests/Correct3.txt",
        // "tests/Correct2.txt",
        // "tests/Correct3.txt",
        // "tests/Incorrect1.txt",
        // "tests/Incorrect2.txt",
        // "tests/Incorrect3.txt",
        // "tests/Incorrect4.txt",
        // "tests/Incorrect5.txt",
    };

    for (const auto& sourceFile : testFiles) {
        std::cout << "Компиляция: " << sourceFile << "\n";

        Lexer   lexer(sourceFile, keywordsFile);
        Semanter sem;
        Poliz   poliz;

        Parser parser(lexer, sem, poliz);

        if (parser.parseProgram()) {
            std::cout << "Разбор завершён успешно\n";

            poliz.dump(std::cout);

            std::cout << "VM start\n";
            InputBuffer input(std::cin);

            VM vm(poliz, input);
            vm.run();
        }
    }

    return 0;
}