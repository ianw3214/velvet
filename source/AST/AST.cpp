#include "AST.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <algorithm>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include <assert.hpp>

static std::unique_ptr<llvm::LLVMContext> sContext;
static std::unique_ptr<llvm::Module> sModule;
static std::unique_ptr<llvm::IRBuilder<>> sBuilder;
static std::unordered_map<std::string, llvm::AllocaInst*> sNamedValues;

/////////////////////////////////////////////////////////////////////////////////////////
// DEBUG
void initLLVM() {
	sContext = std::make_unique<llvm::LLVMContext>();
	ASSERT(sContext, "LLVM Context should not be null");
	sModule = std::make_unique<llvm::Module>("foo", *sContext);
	ASSERT(sModule, "LLVM module should not be null");
	sBuilder = std::make_unique<llvm::IRBuilder<>>(*sContext);
	ASSERT(sBuilder, "LLVM Builder should not be null");
}

void printLLVM() {
	sModule->print(llvm::errs(), nullptr);

	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	std::string targetError;
	const std::string targetTriple = llvm::sys::getDefaultTargetTriple();
	const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, targetError);
	ASSERT(target, "LLVM target not found", targetTriple);

	const std::string CPU = "generic";
	const std::string features = "";
	llvm::TargetOptions options;
	llvm::Optional<llvm::Reloc::Model> rm;
	llvm::TargetMachine* targetMachine = target->createTargetMachine(targetTriple, CPU, features, options, rm);

	sModule->setDataLayout(targetMachine->createDataLayout());
	sModule->setTargetTriple(targetTriple);

	std::string fileName = "output.o";
	std::error_code errorCode;
	llvm::raw_fd_ostream destination(fileName, errorCode, llvm::sys::fs::OF_None);
	ASSERT(errorCode, "Could not open file", errorCode.message());

	llvm::legacy::PassManager passManager;
	llvm::CodeGenFileType fileType = llvm::CGFT_ObjectFile;
	if (targetMachine->addPassesToEmitFile(passManager, destination, nullptr, fileType)) {
		llvm::errs() << "TargetMachine can't emit file of this type: ";
		return;
	}
	passManager.run(*sModule);
	destination.flush();
}
/////////////////////////////////////////////////////////////////////////////////////////
// HELPER

namespace {
	llvm::Type* GetRawLLVMType(Token typeClass) {
		if (typeClass == Token::TYPE_I32) {
			return llvm::Type::getInt32Ty(*sContext);
		}
		if (typeClass == Token::TYPE_F32) {
			return llvm::Type::getFloatTy(*sContext);
		}
		if (typeClass == Token::TYPE_BOOL) {
			return llvm::Type::getInt1Ty(*sContext);
		}
		return nullptr;
	}

	llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* func, const std::string& varName, Token type) {
		llvm::IRBuilder<> tempBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
		return tempBuilder.CreateAlloca(GetRawLLVMType(type), 0, varName.c_str());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

llvm::Value* IdentifierNode::Codegen() {
	llvm::AllocaInst* value = sNamedValues[mIdentifier];
	if (!value) {
		// TODO: Error
		return nullptr;
	}
	else {
		return sBuilder->CreateLoad(value->getAllocatedType(), value, mIdentifier.c_str());
	}
}

llvm::Value* NumberNode::Codegen() {
	// TODO: This should be converted during parse?
	// TODO: This should have different llvm type depending on context
	// TODO: There is probably a better way of doing this
	if (mNumber.find('.') == std::string::npos) {
		// TODO: Set numbits based on context
		return llvm::ConstantInt::get(*sContext, llvm::APInt(32, std::stoi(mNumber)));
	}
	else {
		return llvm::ConstantFP::get(*sContext, llvm::APFloat(std::stof(mNumber)));
	}
}

llvm::Value* BlockExpressionNode::Codegen() {
	// statement list could be empty if the block expression just contains 1 single expression
	return mExpressionList->Codegen();
}

llvm::Value* IfExpressionNode::Codegen() {
	llvm::Value* condition = mConditionNode->Codegen();
	if (!condition) {
		return nullptr;
	}

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
	llvm::PHINode* phi = sBuilder->CreatePHI(thenVal->getType(), 2, "iftmp");
	phi->addIncoming(thenVal, thenBasicBlock);
	if (elseVal) {
		phi->addIncoming(elseVal, elseBasicBlock);
	}

	return phi;
}

llvm::Value* LoopExpressionNode::Codegen() {
	llvm::Function* func = sBuilder->GetInsertBlock()->getParent();
	llvm::BasicBlock* loopBlock = llvm::BasicBlock::Create(*sContext, "loop", func);

	sBuilder->CreateBr(loopBlock);
	sBuilder->SetInsertPoint(loopBlock);

	llvm::Value* blockExpressionValue = mBlockNode->Codegen();
	sBuilder->CreateBr(loopBlock);

	// TODO: This should return last expression of the loop?
	//  - Also need to handle break and return statements?
	//  - last expression of loop doesn't make sense i think...
	//    - unless some sort of language construct gets added for this? idk
	return blockExpressionValue;
}

llvm::Value* RelationalExpressionNode::Codegen() {
	return nullptr;
}

llvm::Value* BinaryExpressionNode::Codegen() {
	llvm::Value* left = mLeft->Codegen();
	llvm::Value* right = mRight->Codegen();
	if (!left || !right) {
		// TODO: Error
		return nullptr;
	}
	if (left->getType() != right->getType()) {
		// TODO: Error
		return nullptr;
	}
	switch (mOperator) {
	case Token::PLUS: {
		if (left->getType()->isFloatTy()) {
			return sBuilder->CreateFAdd(left, right, "addtmp");
		}
		else {
			return sBuilder->CreateAdd(left, right, "addtmp");
		}
	} break;
	case Token::MINUS: {
		if (left->getType()->isFloatTy()) {
			return sBuilder->CreateFSub(left, right, "subtmp");
		}
		else {
			return sBuilder->CreateSub(left, right, "subtmp");
		}
	} break;
	case Token::MULTIPLY: {
		if (left->getType()->isFloatTy()) {
			return sBuilder->CreateFMul(left, right, "multmp");
		}
		else {
			return sBuilder->CreateMul(left, right, "multmp");
		}
	} break;
	case Token::DIVIDE: {
		if (left->getType()->isFloatTy()) {
			return sBuilder->CreateFDiv(left, right, "divtmp");
		}
		else {
			// TODO: Figure out which div to use here
			return sBuilder->CreateFDiv(left, right, "divtmp");
		}
	} break;
	}
	return nullptr;
}

llvm::Value* VariableDeclarationNode::Codegen() {
	llvm::Function* parent = sBuilder->GetInsertBlock()->getParent();
	llvm::Value* initVal = nullptr;
	if (mExpression) {
		initVal = mExpression->Codegen();
		if (!initVal) {
			//TODO: Error
			return nullptr;
		}
	}
	else {
		initVal = llvm::ConstantFP::get(*sContext, llvm::APFloat(0.0f));
	}
	llvm::AllocaInst* allocaInst = CreateEntryBlockAlloca(parent, mIdentifier, mType->mTypeClass);
	sBuilder->CreateStore(initVal, allocaInst);

	sNamedValues[mIdentifier] = allocaInst;
	return nullptr;
}

llvm::Value* AssignmentExpressionNode::Codegen() {
	llvm::Value* variable = sNamedValues[mIdentifier];
	if (!variable) {
		// TODO: Error
	}
	llvm::Value* value = mExpression->Codegen();
	sBuilder->CreateStore(value, variable);
	return nullptr;
}

llvm::Value* ExpressionListNode::Codegen() {
	llvm::Value* last = nullptr;
	for (ASTNode* node : mExpressions) {
		ASSERT(node, "Empty node should not be in expression list");
		last = node->Codegen();
	}
	return last;
}

llvm::Value* FunctionDeclNode::Codegen() {
	// Transform the parameter type list into LLVM types
	std::vector<llvm::Type*> paramTypes;
	if (mParamList) {
		std::transform(mParamList->mParams.cbegin(), mParamList->mParams.cend(), std::back_inserter(paramTypes), [](FunctionParamNode* rawParam) {
			return GetRawLLVMType(rawParam->mType->mTypeClass);
		});
	}

	llvm::FunctionType* funcType = llvm::FunctionType::get(GetRawLLVMType(mType->mTypeClass), paramTypes, false);
	llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, mName, sModule.get());

	// Create basic block to start insertion into
	llvm::BasicBlock* basicBlock = llvm::BasicBlock::Create(*sContext, "entry", func);
	sBuilder->SetInsertPoint(basicBlock);

	unsigned int index = 0;
	for (auto& arg : func->args()) {
		FunctionParamNode* param = mParamList->mParams[index];
		llvm::AllocaInst* allocaInst = CreateEntryBlockAlloca(func, param->mName, param->mType->mTypeClass);
		sBuilder->CreateStore(&arg, allocaInst);
		sNamedValues[param->mName] = allocaInst;
		index++;
	}

	if (llvm::Value* value = mBlockExpr->Codegen()) {
		sBuilder->CreateRet(value);

		llvm::verifyFunction(*func);
		return func;
	}

	// handle error
	// func->eraseFromParent();
	return nullptr;
}

llvm::Value* FunctionParamNode::Codegen() {
	return nullptr;
}

llvm::Value* FunctionParamListNode::Codegen() {
	return nullptr;
}

llvm::Value* TypeNode::Codegen() {
	return nullptr;
}