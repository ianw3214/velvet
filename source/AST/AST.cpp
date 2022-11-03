#include "AST.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

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
	// statement list could be empty if the block expression just contains 1 single expression
	if (mStatementList) {
		mStatementList->Codegen();
	}
	return mExprNode->Codegen();
}

llvm::Value* IfExpressionNode::Codegen() {
	llvm::Value* condition = mConditionNode->Codegen();
	if (!condition) {
		return nullptr;
	}
	// TODO: Skip the compare to 0 since that's not necessary in this language
	condition = sBuilder->CreateFCmpONE(condition, llvm::ConstantFP::get(*sContext, llvm::APFloat(0.0)), "ifcond");

	llvm::Function* parentFunc = sBuilder->GetInsertBlock()->getParent();
	llvm::BasicBlock* thenBasicBlock = llvm::BasicBlock::Create(*sContext, "then", parentFunc);
	llvm::BasicBlock* elseBasicBlock = llvm::BasicBlock::Create(*sContext, "else");
	llvm::BasicBlock* mergeBasicBlock = llvm::BasicBlock::Create(*sContext, "ifcont");
	sBuilder->CreateCondBr(condition, thenBasicBlock, elseBasicBlock);

	sBuilder->SetInsertPoint(thenBasicBlock);
	llvm::Value* thenVal = mThenNode->Codegen();
	if (!thenVal) {
		return nullptr;
	}
	sBuilder->CreateBr(mergeBasicBlock);
	thenBasicBlock = sBuilder->GetInsertBlock();

	llvm::Value* elseVal = nullptr;
	if (mElseNode) {
		parentFunc->getBasicBlockList().push_back(elseBasicBlock);
		sBuilder->SetInsertPoint(elseBasicBlock);

		elseVal = mElseNode->Codegen();
		if (!elseVal) {
			return nullptr;
		}
		sBuilder->CreateBr(mergeBasicBlock);
		elseBasicBlock = sBuilder->GetInsertBlock();
	}

	parentFunc->getBasicBlockList().push_back(mergeBasicBlock);
	sBuilder->SetInsertPoint(mergeBasicBlock);
	llvm::PHINode* phi = sBuilder->CreatePHI(llvm::Type::getDoubleTy(*sContext), 2, "iftmp");
	phi->addIncoming(thenVal, thenBasicBlock);
	if (elseVal) {
		phi->addIncoming(elseVal, elseBasicBlock);
	}

	return phi;
}

llvm::Value* LoopExpressionNode::Codegen() {
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

	// Create basic block to start insertion into
	llvm::BasicBlock* basicBlock = llvm::BasicBlock::Create(*sContext, "entry", func);
	sBuilder->SetInsertPoint(basicBlock);

	// TODO: Record function arguments in named values map

	if (llvm::Value* value = mBlockExpr->Codegen()) {
		sBuilder->CreateRet(value);

		llvm::verifyFunction(*func);
		return func;
	}

	// handle error
	// func->eraseFromParent();
	return nullptr;
}