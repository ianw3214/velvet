#include <iostream>
#include "llvm/IR/LLVMContext.h"

int main() {
    llvm::LLVMContext context;
    std::cout << &context << std::endl;
    return 0;
};