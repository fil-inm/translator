#include "lexer.hpp"
#include <iostream>
#include <unordered_map>

Lexer::Lexer(const std::string &filename, std::string keywordFile) {
    file.open(filename);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open " << filename << std::endl;
        eof = true;
        return;
    }

    loadKeywordsFromFile(keywordFile);

    readChar();
    nextLexem();
}

Lexer::Lexer(Lexer &&other) noexcept {
    file = std::move(other.file);
    keywords = std::move(other.keywords);
    currentChar = other.currentChar;
    eof = other.eof;
    line = other.line;
    column = other.column;
    currentToken = std::move(other.currentToken);
}

Lexer &Lexer::operator=(Lexer &&other) noexcept {
    if (this != &other) {
        file = std::move(other.file);
        keywords = std::move(other.keywords);
        currentChar = other.currentChar;
        eof = other.eof;
        line = other.line;
        column = other.column;
        currentToken = std::move(other.currentToken);
    }
    return *this;
}

void Lexer::readChar() {
    if (!file.get(currentChar)) {
        eof = true;
        currentChar = '\0';
    } else {
        if (currentChar == '\n') {
            line++;
            column = 0;
        } else {
            column++;
        }
    }
}

void Lexer::skipWhitespaceAndComments() {
    while (!eof) {
        while (std::isspace(static_cast<unsigned char>(currentChar)))
            readChar();

        if (currentChar == '/' && file.peek() == '/') {
            while (!eof && currentChar != '\n')
                readChar();
            continue;
        }

        if (currentChar == '/' && file.peek() == '*') {
            readChar();
            readChar();
            bool closed = false;
            while (!eof) {
                if (currentChar == '*' && file.peek() == '/') {
                    readChar();
                    readChar();
                    closed = true;
                    break;
                }
                readChar();
            }
            if (!closed)
                std::cerr << "Warning: unterminated block comment\n";
            continue;
        }

        break;
    }
}

void Lexer::loadKeywordsFromFile(const std::string &filename) {
    std::ifstream kwFile(filename);
    if (!kwFile.is_open()) {
        std::cerr << "Error: cannot open keywords file: " << filename << std::endl;
        return;
    }

    std::string word;
    while (kwFile >> word)
        keywords.insert(word);
}

Token Lexer::makeToken(Token::Type type, const std::string &value, int l, int c) {
    Token t;
    t.type = type;
    t.lexeme = value;
    t.pos = {l, c};
    return t;
}

const Token &Lexer::currentLexeme() const {
    return currentToken;
}

Token Lexer::nextLexem() {
    skipWhitespaceAndComments();
    if (eof) {
        currentToken = makeToken(Token::Type::EndOfFile, "", line, column);
        return currentToken;
    }

    if (std::isalpha(static_cast<unsigned char>(currentChar)) || currentChar == '_') {
        currentToken = readIdentifierOrKeyword();
    } else if (std::isdigit(static_cast<unsigned char>(currentChar))) {
        currentToken = readNumber();
    } else if (currentChar == '\'') {
        currentToken = readCharLiteral();
    } else if (currentChar == '"') {
        currentToken = readStringLiteral();
    } else {
        currentToken = readOperatorOrDelimiter();
    }

    return currentToken;
}

Token Lexer::peekNextLexeme() {
    std::streampos oldPos = file.tellg();
    int oldLine = line;
    int oldColumn = column;
    char oldChar = currentChar;
    bool oldEof = eof;
    Token oldToken = currentToken;

    Token next = const_cast<Lexer*>(this)->nextLexem();

    file.clear();
    file.seekg(oldPos);
    line = oldLine;
    column = oldColumn;
    currentChar = oldChar;
    eof = oldEof;
    currentToken = oldToken;

    return next;
}

Token Lexer::readIdentifierOrKeyword() {
    std::string word;
    int startLine = line;
    int startCol = column == 0 ? 1 : column;

    while (std::isalnum(static_cast<unsigned char>(currentChar)) || currentChar == '_') {
        word += currentChar;
        readChar();
    }

    if (keywords.search(word)) {
        if (word == "int") return makeToken(Token::Type::KwInt, word, startLine, startCol);
        if (word == "char") return makeToken(Token::Type::KwChar, word, startLine, startCol);
        if (word == "bool") return makeToken(Token::Type::KwBool, word, startLine, startCol);
        if (word == "float") return makeToken(Token::Type::KwFloat, word, startLine, startCol);
        if (word == "void") return makeToken(Token::Type::KwVoid, word, startLine, startCol);

        if (word == "class") return makeToken(Token::Type::KwClass, word, startLine, startCol);
        if (word == "constructor") return makeToken(Token::Type::KwConstructor, word, startLine, startCol);
        if (word == "main") return makeToken(Token::Type::KwMain, word, startLine, startCol);

        if (word == "if") return makeToken(Token::Type::KwIf, word, startLine, startCol);
        if (word == "else") return makeToken(Token::Type::KwElse, word, startLine, startCol);
        if (word == "while") return makeToken(Token::Type::KwWhile, word, startLine, startCol);
        if (word == "for") return makeToken(Token::Type::KwFor, word, startLine, startCol);
        if (word == "return") return makeToken(Token::Type::KwReturn, word, startLine, startCol);
        if (word == "break") return makeToken(Token::Type::KwBreak, word, startLine, startCol);
        if (word == "continue") return makeToken(Token::Type::KwContinue, word, startLine, startCol);

        if (word == "print") return makeToken(Token::Type::KwPrint, word, startLine, startCol);
        if (word == "read") return makeToken(Token::Type::KwRead, word, startLine, startCol);

        if (word == "true") return makeToken(Token::Type::KwTrue, word, startLine, startCol);
        if (word == "false") return makeToken(Token::Type::KwFalse, word, startLine, startCol);

    }

    return makeToken(Token::Type::Identifier, word, startLine, startCol);
}

Token Lexer::readNumber() {
    int startLine = line;
    int startCol = column == 0 ? 1 : column;
    std::string num;
    bool seenDot = false;

    while (std::isdigit(static_cast<unsigned char>(currentChar)) ||
           (!seenDot && currentChar == '.')) {
        if (currentChar == '.')
            seenDot = true;
        num += currentChar;
        readChar();
    }

    if (seenDot)
        return makeToken(Token::Type::FloatLiteral, num, startLine, startCol);
    else
        return makeToken(Token::Type::IntegerLiteral, num, startLine, startCol);
}

Token Lexer::readCharLiteral() {
    int startLine = line;
    int startCol = column == 0 ? 1 : column;

    readChar();  // пропускаем открывающую '
    std::string content;

    if (eof || currentChar == '\n' || currentChar == '\'') {
        std::cerr << "Error: empty char literal at " << startLine << ":" << startCol << "\n";
        return makeToken(Token::Type::CharLiteral, "", startLine, startCol);
    }

    if (currentChar == '\\') {
        content += currentChar;
        readChar();
        if (!eof) {
            content += currentChar;
            readChar();
        }
    } else {
        content += currentChar;
        readChar();
    }

    if (currentChar != '\'') {
        std::cerr << "Error: unterminated char literal at " << startLine << ":" << startCol << "\n";
    } else {
        readChar();
    }

    return makeToken(Token::Type::CharLiteral, content, startLine, startCol);
}

Token Lexer::readStringLiteral() {
    int startLine = line;
    int startCol = column == 0 ? 1 : column;

    readChar();
    std::string content;
    bool escaped = false;

    while (!eof) {
        if (escaped) {
            switch (currentChar) {
                case 'n': content += '\n'; break;
                case 't': content += '\t'; break;
                case 'r': content += '\r'; break;
                case '\\': content += '\\'; break;
                case '"': content += '"'; break;
                default: content += currentChar; break;
            }
            escaped = false;
        } else if (currentChar == '\\') {
            escaped = true;
        } else if (currentChar == '"') {
            readChar();
            return makeToken(Token::Type::StringLiteral, content, startLine, startCol);
        } else if (currentChar == '\n' || eof) {
            std::cerr << "Warning: unterminated string literal at "
                      << startLine << ":" << startCol << "\n";
            break;
        } else {
            content += currentChar;
        }
        readChar();
    }

    return makeToken(Token::Type::StringLiteral, content, startLine, startCol);
}

Token Lexer::readOperatorOrDelimiter() {
    int startLine = line;
    int startCol = column == 0 ? 1 : column;
    std::string op(1, currentChar);
    char next = file.peek();

    if (!eof) {
        std::string two = op + next;
        if (two == "==") {
            readChar();
            readChar();
            return makeToken(Token::Type::EqualEqual, "==", startLine, startCol);
        }
        if (two == "!=") {
            readChar();
            readChar();
            return makeToken(Token::Type::NotEqual, "!=", startLine, startCol);
        }
        if (two == "<=") {
            readChar();
            readChar();
            return makeToken(Token::Type::LessEqual, "<=", startLine, startCol);
        }
        if (two == ">=") {
            readChar();
            readChar();
            return makeToken(Token::Type::GreaterEqual, ">=", startLine, startCol);
        }
        if (two == "++") {
            readChar();
            readChar();
            return makeToken(Token::Type::PlusPlus, "++", startLine, startCol);
        }
        if (two == "--") {
            readChar();
            readChar();
            return makeToken(Token::Type::MinusMinus, "--", startLine, startCol);
        }
        if (two == "&&") {
            readChar();
            readChar();
            return makeToken(Token::Type::AmpAmp, "&&", startLine, startCol);
        }
        if (two == "||") {
            readChar();
            readChar();
            return makeToken(Token::Type::PipePipe, "||", startLine, startCol);
        }
        if (two == "<<") {
            readChar();
            readChar();
            return makeToken(Token::Type::Shl, "<<", startLine, startCol);
        }
        if (two == ">>") {
            readChar();
            readChar();
            return makeToken(Token::Type::Shr, ">>", startLine, startCol);
        }
    }

    char c = currentChar;
    readChar();
    switch (c) {
        case '(': return makeToken(Token::Type::LParen, "(", startLine, startCol);
        case ')': return makeToken(Token::Type::RParen, ")", startLine, startCol);
        case '{': return makeToken(Token::Type::LBrace, "{", startLine, startCol);
        case '}': return makeToken(Token::Type::RBrace, "}", startLine, startCol);
        case '[': return makeToken(Token::Type::LBracket, "[", startLine, startCol);
        case ']': return makeToken(Token::Type::RBracket, "]", startLine, startCol);
        case ',': return makeToken(Token::Type::Comma, ",", startLine, startCol);
        case ';': return makeToken(Token::Type::Semicolon, ";", startLine, startCol);
        case '.': return makeToken(Token::Type::Dot, ".", startLine, startCol);
        case '`': return makeToken(Token::Type::Backtick, "`", startLine, startCol);
        case '+': return makeToken(Token::Type::Plus, "+", startLine, startCol);
        case '-': return makeToken(Token::Type::Minus, "-", startLine, startCol);
        case '*': return makeToken(Token::Type::Asterisk, "*", startLine, startCol);
        case '/': return makeToken(Token::Type::Slash, "/", startLine, startCol);
        case '%': return makeToken(Token::Type::Percent, "%", startLine, startCol);
        case '&': return makeToken(Token::Type::Ampersand, "&", startLine, startCol);
        case '|': return makeToken(Token::Type::VerticalBar, "|", startLine, startCol);
        case '^': return makeToken(Token::Type::Caret, "^", startLine, startCol);
        case '!': return makeToken(Token::Type::Exclamation, "!", startLine, startCol);
        case '~': return makeToken(Token::Type::Tilde, "~", startLine, startCol);
        case '=': return makeToken(Token::Type::Assign, "=", startLine, startCol);
        case '<': return makeToken(Token::Type::Less, "<", startLine, startCol);
        case '>': return makeToken(Token::Type::Greater, ">", startLine, startCol);
        default:
            std::cerr << "Unknown token '" << c << "' at " << startLine << ":" << startCol << "\n";
            return makeToken(Token::Type::Identifier, std::string(1, c), startLine, startCol);
    }
}
