#include <iostream>
#include "llvm/IR/LLVMContext.h"

#include "lexer/lexer.h"

int main() {
    llvm::LLVMContext context;
    std::cout << &context << std::endl;

    Lexer::Initialize();

    Lexer::LoadString("if something 135 then something else something");

    Lexer::Lexeme lexeme = Lexer::getLexeme();
    
    while (lexeme.token != Token::TOKEN_EOF && lexeme.token != Token::INVALID) {
        if (lexeme.token == Token::INVALID) {
            std::cout << "AH FUCK";
        }
        if (lexeme.token == Token::WHITESPACE) {
            lexeme = Lexer::getLexeme();
            continue;
        }
        if (lexeme.token == Token::IF) {
            std::cout << "IF";
        }
        if (lexeme.token == Token::THEN) {
            std::cout << "THEN";
        }
        if (lexeme.token == Token::ELSE) {
            std::cout << "ELSE";
        }
        if (lexeme.token == Token::ID) {
            std::cout << "ID";
        }
        if (lexeme.token == Token::NUM) {
            std::cout << "NUM";
        }
        std::cout << ": " << lexeme.symbol << std::endl;
        lexeme = Lexer::getLexeme();
    }

    return 0;
};