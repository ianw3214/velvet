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

    Parser::Parse();

    return 0;
};