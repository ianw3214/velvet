#include "builder.h"

#include <string>
#include <iostream>
#include <optional>

#include "llvm/MC/TargetRegistry.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"

TargetBuilder::TargetBuilder() {
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    mTargetTriple = llvm::sys::getDefaultTargetTriple();
    std::cout << mTargetTriple << '\n';

    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(mTargetTriple, error);
    if (!target) {
        // TODO: Error
        llvm::errs() << error;
        return;
    }

    const std::string cpu = "generic";
    const std::string features = "";
    llvm::TargetOptions options;
    llvm::Optional<llvm::Reloc::Model> RM = llvm::Optional<llvm::Reloc::Model>();
    mTargetMachine = target->createTargetMachine(mTargetTriple, cpu, features, options, RM);


}

bool TargetBuilder::buildModule(std::unique_ptr<llvm::Module>& module) {
    module->setDataLayout(mTargetMachine->createDataLayout());
    module->setTargetTriple(mTargetTriple);

    std::string fileName = "output.o";
    std::error_code errorCode;
    llvm::raw_fd_ostream destination(fileName, errorCode, llvm::sys::fs::OF_None);
    if (errorCode) {
        // TODO: Error
        llvm::errs() << "Could not open file: " << errorCode.message();
        return false;
    }

    llvm::legacy::PassManager passManager;
    llvm::CodeGenFileType fileType = llvm::CGFT_ObjectFile;
    if (mTargetMachine->addPassesToEmitFile(passManager, destination, nullptr, fileType)) {
        // TODO: Error
        llvm::errs() << "Target machine can't emit file of this type\n";
        return false;
    }

    passManager.run(*module.get());
    destination.flush();
}