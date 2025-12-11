#include "lexer.hpp"
#include "semanter.hpp"
#include "parser.hpp"
#include "poliz.hpp"
#include "vm.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    const std::string keywordsFile = "keywords.txt";

    std::vector<std::string> testFiles = {
        "tests/Correct2.txt",
    };

    for (const auto& sourceFile : testFiles) {
        std::cout << "=== Проверяю: " << sourceFile << " ===\n";

        Lexer   lexer(sourceFile, keywordsFile);
        Semanter sem;
        Poliz   poliz;

        Parser parser(lexer, sem, poliz);

        if (parser.parseProgram()) {
            std::cout << "Разбор завершён успешно\n";

            poliz.emit(Poliz::Op::HALT);

            poliz.dump(std::cout);

            std::cout << "--- VM output ---\n";
            VM vm(poliz);
            vm.run();
            std::cout << "-----------------\n\n";
        }
    }

    return 0;
}