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

static std::unique_ptr<llvm::LLVMContext> sContext;
static std::unique_ptr<llvm::Module> sModule;
static std::unique_ptr<llvm::IRBuilder<>> sBuilder;
static std::unordered_map<std::string, llvm::AllocaInst*> sNamedValues;

/////////////////////////////////////////////////////////////////////////////////////////
// DEBUG
void initLLVM() {
	sContext = std::make_unique<llvm::LLVMContext>();
	sModule = std::make_unique<llvm::Module>("foo", *sContext);

	sBuilder = std::make_unique<llvm::IRBuilder<>>(*sContext);
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
	if (!target) {
		// TODO: Error handling
		return;
	}

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
	if (errorCode) {
		llvm::errs() << "Could not open file: " << errorCode.message();
		return;
	}

	llvm::legacy::PassManager pass;
	llvm::CodeGenFileType fileType = llvm::CGFT_ObjectFile;
	if (targetMachine->addPassesToEmitFile(pass, destination, nullptr, fileType)) {
		llvm::errs() << "TargetMachine can't emit file of this type: ";
		return;
	}
	pass.run(*sModule);
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
	}
	return sBuilder->CreateLoad(value->getAllocatedType(), value, mIdentifier.c_str());
}

llvm::Value* NumberNode::Codegen() {
	// TODO: This should be converted during parse?
	// TODO: This should have different llvm type depending on context
	return llvm::ConstantFP::get(*sContext, llvm::APFloat(std::stof(mNumber)));
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
		if (left->getType()->isFloatTy()) {
			return sBuilder->CreateFAdd(left, right, "addtmp");
		}
		else {
			return sBuilder->CreateAdd(left, right, "addtmp");
		}
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
	TypeNode* type = dynamic_cast<TypeNode*>(mType);
	llvm::AllocaInst* allocaInst = CreateEntryBlockAlloca(parent, mIdentifier, type->mTypeClass);
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
		last = node->Codegen();
	}
	return last;
}

llvm::Value* FunctionDeclNode::Codegen() {
	// TODO: Actually handle what the types should be
	FunctionParamListNode* params = dynamic_cast<FunctionParamListNode*>(mParamList);
	std::vector<llvm::Type*> paramTypes;
	if (params) {
		std::transform(params->mParams.cbegin(), params->mParams.cend(), std::back_inserter(paramTypes), [](ASTNode* rawParam) {
			FunctionParamNode* param = dynamic_cast<FunctionParamNode*>(rawParam);
			TypeNode* type = dynamic_cast<TypeNode*>(param->mType);
			return GetRawLLVMType(type->mTypeClass);
		});
	}

	TypeNode* returnType = dynamic_cast<TypeNode*>(mType);
	llvm::FunctionType* funcType = llvm::FunctionType::get(GetRawLLVMType(returnType->mTypeClass), paramTypes, false);
	llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, mName, sModule.get());

	// Create basic block to start insertion into
	llvm::BasicBlock* basicBlock = llvm::BasicBlock::Create(*sContext, "entry", func);
	sBuilder->SetInsertPoint(basicBlock);

	unsigned int index = 0;
	for (auto& arg : func->args()) {
		FunctionParamNode* param = dynamic_cast<FunctionParamNode*>(params->mParams[index]);
		TypeNode* type = dynamic_cast<TypeNode*>(param->mType);
		llvm::AllocaInst* allocaInst = CreateEntryBlockAlloca(func, param->mName, type->mTypeClass);
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