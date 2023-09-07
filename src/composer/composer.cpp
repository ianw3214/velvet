#include "composer.h"

#include <fstream>
#include <sstream>

#include "error/errorHandler.h"

#include "parser/parser.h"
#include "codegen/codegen.h"
#include "builder/builder.h"

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

//////////////////////////////////////////////////////////////
// This stuff should be platform specific
#include <windows.h>
//////////////////////////////////////////////////////////////

namespace {
    std::string _sourceToObjectFileName(const std::string& sourceName) {
        constexpr size_t velvetSourceFileExtensionSize = 2;
        return sourceName.substr(0, sourceName.size() - velvetSourceFileExtensionSize) + "o";
    }
}

Composer::Composer(ErrorHandler& errorHandler) 
    : mInputFiles() 
    , mObjectFiles() 
    , mErrorHandler(errorHandler) {

}

void Composer::addInputFile(const std::string& fileName) {
    mInputFiles.emplace_back(fileName);
}

void Composer::buildAllFiles() {
    // optimization passes
    //  - consider moving this to a separate file
    llvm::LoopAnalysisManager loopAnalysis;
    llvm::FunctionAnalysisManager funcAnalysis;
    llvm::CGSCCAnalysisManager CGSCCAnalysis;
    llvm::ModuleAnalysisManager moduleAnalysis;

    llvm::PassBuilder passBuilder;
    passBuilder.registerLoopAnalyses(loopAnalysis);
    passBuilder.registerFunctionAnalyses(funcAnalysis);
    passBuilder.registerCGSCCAnalyses(CGSCCAnalysis);
    passBuilder.registerModuleAnalyses(moduleAnalysis);
    passBuilder.crossRegisterProxies(loopAnalysis, funcAnalysis, CGSCCAnalysis, moduleAnalysis);
    // TODO: Optimization level should be passed in
    llvm::ModulePassManager modulePassManager = passBuilder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O0);

    for (const std::string& filename : mInputFiles) {
        std::ifstream inputFile(filename);
        if (inputFile.is_open()) {
            std::stringstream buffer;
            buffer << inputFile.rdbuf();
            const std::string& contents = buffer.str();
            
            // Lexing/parsing------------
            Parser parser(contents, mErrorHandler);
            std::vector<FunctionDefinitionNode>& topLevelFuncs = parser.parseAll();
            if (mErrorHandler.hasError()) {
                continue;
            }

            // codegen------------
            CodeGenerator generator(mErrorHandler);
            for (FunctionDefinitionNode& func : topLevelFuncs) {
                llvm::Function* funcIR = generator.generateFunctionCode(func);
            }
            if (mErrorHandler.hasError()) {
                continue;
            }
            modulePassManager.run(*generator.getModule().get(), moduleAnalysis);
            // TODO: Print only via debug flag
            // funcIR->print(llvm::errs());
            generator.getModule()->print(llvm::errs(), nullptr);

            // object file output---------------
            TargetBuilder builder;
            std::string outputFileName = _sourceToObjectFileName(filename);
            builder.buildModule(generator.getModule(), outputFileName);
            mObjectFiles.emplace_back(std::move(outputFileName));
        }
    }
}

void Composer::generateExecutable() {
    //////////////////////////////////////////////////////////////
    // This stuff should be platform specific
    const wchar_t* clang = L"clang";
    // TODO: Lots of stuff to improve here
    //  - custom output name
    //  - perhaps want to allow passing '-v' to clang
    std::wstring commandLine = std::wstring(clang) + L" -o main.exe";
    for (const std::string& filename : mObjectFiles) {
        commandLine += L" " + std::wstring(filename.begin(), filename.end());
    }

    // CreateProcess parameters
    STARTUPINFO startupInfo = { sizeof(startupInfo) };
    PROCESS_INFORMATION processInfo;
    // Create the process
    if (CreateProcess(nullptr, const_cast<LPWSTR>(commandLine.c_str()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo)) {
        WaitForSingleObject(processInfo.hProcess, INFINITE);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }
    else {
        mErrorHandler.logError("Failed to start clang linking process");
    }
    //////////////////////////////////////////////////////////////
}