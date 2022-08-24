#include <iostream>
#include "llvm/IR/LLVMContext.h"

#include "common.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

int main() {
    llvm::LLVMContext context;
    std::cout << &context << std::endl;

    // Lexer::LoadString("if something 135 then something else something");
    Lexer::LoadString("a > test <> 100 = 1000");

    auto getTokenCallback = []() {
        Lexeme next = Lexer::getLexeme();
        while (next.token == Token::WHITESPACE) {
            next = Lexer::getLexeme();
        }
        // hard coded lmao
        if (next.symbol == "<" || next.symbol == "<=" || next.symbol == "="
            || next.symbol == "<>" || next.symbol == ">" || next.symbol == ">=") 
        {
            next.token = Token::RELOP;
        }
        return next;
    };

    Parser::Parse(getTokenCallback);

    return 0;
};