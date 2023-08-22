#pragma once

#include <memory>

#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"

class TargetBuilder {
    std::string mTargetTriple = "";
    llvm::TargetMachine* mTargetMachine = nullptr;
public:
    TargetBuilder();

    bool buildModule(std::unique_ptr<llvm::Module>& module, const std::string& fileName);
};