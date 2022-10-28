#include "AST.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"

static std::unique_ptr<llvm::LLVMContext> sContext;
static std::unique_ptr<llvm::Module> sModule;
static std::unique_ptr<llvm::IRBuilder<>> sBuilder;
static std::unordered_map<std::string, llvm::Value*> sNamedValues;

/////////////////////////////////////////////////////////////////////////////////////////
// DEBUG
void initLLVM() {
	sContext = std::make_unique<llvm::LLVMContext>();
	sModule = std::make_unique<llvm::Module>("foo", *sContext);

	sBuilder = std::make_unique<llvm::IRBuilder<>>(*sContext);
}

void printLLVM() {
	sModule->print(llvm::errs(), nullptr);
}
/////////////////////////////////////////////////////////////////////////////////////////

llvm::Value* IdentifierNode::Codegen() {
	llvm::Value* value = sNamedValues[mIdentifier];
	if (!value) {
		// TODO: Error
	}
	return value;
}

llvm::Value* NumberNode::Codegen() {
	// TODO: This should be converted during parse?
	return llvm::ConstantFP::get(*sContext, llvm::APFloat(std::stof(mNumber)));
}

llvm::Value* BlockExpressionNode::Codegen() {
	return nullptr;
}

llvm::Value* IfExpressionNode::Codegen() {
	return nullptr;
}

llvm::Value* RelationalOperatorNode::Codegen() {
	return nullptr;
}

llvm::Value* BinaryOperatorNode::Codegen() {
	llvm::Value* left = mLeft->Codegen();
	llvm::Value* right = mRight->Codegen();
	if (!left || !right) {
		return nullptr;
	}
	switch (mOperator) {
	case Token::PLUS: {
		return sBuilder->CreateFAdd(left, right, "addtmp");
	} break;
	case Token::MINUS: {
		return sBuilder->CreateFAdd(left, right, "subtmp");
	} break;
	case Token::MULTIPLY: {
		return sBuilder->CreateFAdd(left, right, "multmp");
	} break;
		/*
	case Token::DIVIDE: {
		return sBuilder->CreateFAdd(left, right, "divtmp");
	} break;
	*/
	}
	return nullptr;
}

llvm::Value* VariableDeclarationNode::Codegen() {
	return nullptr;
}

llvm::Value* AssignmentStatementNode::Codegen() {
	return nullptr;
}

llvm::Value* StatementListNode::Codegen() {
	for (ASTNode* node : mStatements) {
		node->Codegen();
	}
	return nullptr;
}

llvm::Value* FunctionDeclNode::Codegen() {
	llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getDoublePtrTy(*sContext), false);
	llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, mName, sModule.get());
	return nullptr;
}