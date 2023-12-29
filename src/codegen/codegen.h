#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <utility>

#include "error/errorHandler.h"
#include "parser/ast.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

// TODO: Maybe look to move this to a common struct to share with parser?
struct VariableInfo {
    llvm::AllocaInst* mAlloca;
    Token mRawType;
    bool mIsDecayedArray;
    std::vector<size_t> mArraySize;
};

class CodeGenerator {
    std::unique_ptr<llvm::LLVMContext> mContext;
    std::unique_ptr<llvm::Module> mModule;
    std::unique_ptr<llvm::IRBuilder<>> mBuilder;

    ErrorHandler& mErrorHandler;

    llvm::Type* _getRawLLVMType(Token type) const;
public:
    CodeGenerator(ErrorHandler& handler);

    void setupDefaultFunctions();

    llvm::Value* generateExpressionCode(ExpressionNodeOwner& expressionNode);
    llvm::Function* generateFunctionCode(FunctionDefinitionNode& functionDefinition);

    std::unique_ptr<llvm::Module>& getModule();
private:
    using SymbolTable = std::unordered_map<std::string, VariableInfo>;
    std::vector<SymbolTable> mSymbolStack;
    std::unordered_map<std::string, llvm::Function*> mFunctions;
    std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> mLoopStack;

    llvm::Value* _generateVariableAccess(std::unique_ptr<VariableAccessNode>& varAccess);
    llvm::Value* _generateNumber(std::unique_ptr<NumberNode>& number);
    llvm::Value* _generateScope(std::unique_ptr<ScopeNode>& scope);
    llvm::Value* _generateConditional(std::unique_ptr<ConditionalNode>& conditional);
    llvm::Value* _generateBinaryOperation(std::unique_ptr<BinaryOperationNode>& binaryOperation);
    
    llvm::Value* _generateVariableDefinition(std::unique_ptr<VariableDefinitionNode>& varDef);    
    llvm::Value* _generateAssignment(std::unique_ptr<AssignmentNode>& assignment);
    llvm::Value* _generateLoop(std::unique_ptr<LoopNode>& loop);
    llvm::Value* _generateBreak(std::unique_ptr<BreakNode>& br);

    // special case codegen functions
    llvm::Value* _generateArrayValue(std::unique_ptr<ArrayValueNode>& arrayValue, llvm::AllocaInst* alloca);

private:
    void _pushNewSymbolScope();
    void _popSymbolScope();
    void _addSymbolData(const std::string& varName, llvm::AllocaInst* alloca, Token rawType, bool isDecayedArray, std::vector<size_t> arraySize);
    std::optional<VariableInfo*> _getSymbolData(const std::string& symbol);

    llvm::Value* _getMemLocationFromVariableAccess(VariableAccessNode& varAccess);
};