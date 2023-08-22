#pragma once

#include <memory>
#include <unordered_map>

#include "error/errorHandler.h"
#include "parser/ast.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

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
    std::unordered_map<std::string, llvm::AllocaInst*> mNamedVariables;
    std::unordered_map<std::string, llvm::Function*> mFunctions;

    llvm::Value* _generateVariableAccess(std::unique_ptr<VariableAccessNode>& varAccess);
    llvm::Value* _generateNumber(std::unique_ptr<NumberNode>& number);
    llvm::Value* _generateScope(std::unique_ptr<ScopeNode>& scope);
    llvm::Value* _generateConditional(std::unique_ptr<ConditionalNode>& conditional);
    llvm::Value* _generateBinaryOperation(std::unique_ptr<BinaryOperationNode>& binaryOperation);
    
    llvm::Value* _generateVariableDefinition(std::unique_ptr<VariableDefinitionNode>& varDef);    
    llvm::Value* _generateAssignment(std::unique_ptr<AssignmentNode>& assignment);
};