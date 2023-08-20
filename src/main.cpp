#include <iostream>
#include <fstream>
#include <sstream>

#include "error/errorHandler.h"

#include "parser/parser.h"
#include "codegen/codegen.h"
#include "builder/builder.h"

int main(int argc, char* argv[]) {
    std::string testString = "def func (a : f32, b : f32) @ i32 { if a then{ a } else { b } }";

    // TODO: This logic should probably be put into a separate file
    if (argc > 1) {
        std::string filename(argv[1]);

        std::ifstream inputFile(filename);
        if (inputFile.is_open()) {
            std::stringstream buffer;
            buffer << inputFile.rdbuf();
            testString = buffer.str();
        }
    }

    ErrorHandler errorHandler;
    Parser parser(testString, errorHandler);
    if (errorHandler.hasError()) {
        return 1;
    }
    
    FunctionDefinitionNode func = parser.parseFunctionDefinition();
    CodeGenerator generator(errorHandler);
    llvm::Function* funcIR = generator.generateFunctionCode(func);
    if (errorHandler.hasError()) {
        return 1;
    }

    funcIR->print(llvm::errs());

    TargetBuilder builder;
    builder.buildModule(generator.getModule());

    return 0;
};