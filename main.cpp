#include "lexer.hpp"
#include "parser.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    const std::string keywordsFile = "keywords.txt";

    std::vector<std::string> testFiles = {
        "tests/Correct1.txt",
        // "Correct2.txt",
        // "Correct3.txt",
        // "Incorrect1.txt",
        // "Incorrect2.txt",
        // "Incorrect3.txt",
        // "Incorrect4.txt",
        // "Incorrect5.txt",
    };

    for (const auto& sourceFile : testFiles) {
        std::cout << "=== Проверяю: " << sourceFile << " ===\n";

        Lexer lexer(sourceFile, keywordsFile);
        Semanter sem;
        Poliz   poliz;
        Parser parser(lexer, sem, poliz);

        if (parser.parseProgram()) {
            std::cout << "Разбор завершён успешно\n\n";
            poliz.dump(std::cout); // пока просто посмотрим ПОЛИЗ
        }
    }

    return 0;
}