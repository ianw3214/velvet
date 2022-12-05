#include "AST.h"

#include <memory>
#include <string>
#include <unordered_map>

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
	llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* func, const std::string& varName) {
		llvm::IRBuilder<> tempBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
		return tempBuilder.CreateAlloca(llvm::Type::getDoubleTy(*sContext), 0, varName.c_str());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

llvm::Value* IdentifierNode::Codegen() {
	llvm::Value* value = sNamedValues[mIdentifier];
	if (!value) {
		// TODO: Error
	}
	return sBuilder->CreateLoad(llvm::Type::getDoubleTy(*sContext), value, mIdentifier.c_str());
}

llvm::Value* NumberNode::Codegen() {
	// TODO: This should be converted during parse?
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
	llvm::Function* func = sBuilder->GetInsertBlock()->getParent();
	llvm::BasicBlock* loopBlock = llvm::BasicBlock::Create(*sContext, "loop", func);

	sBuilder->CreateBr(loopBlock);
	sBuilder->SetInsertPoint(loopBlock);

	mBlockNode->Codegen();
	sBuilder->CreateBr(loopBlock);

	// TODO: This should return last expression of the loop?
	//  - Also need to handle break and return statements?
	return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*sContext));
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
	llvm::AllocaInst* allocaInst = CreateEntryBlockAlloca(parent, mIdentifier);
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
	for (ASTNode* node : mExpressions) {
		node->Codegen();
	}
	return nullptr;
}

llvm::Value* FunctionDeclNode::Codegen() {
	// TODO: Actually handle what the types should be
	FunctionParamListNode* params = dynamic_cast<FunctionParamListNode*>(mParamList);
	std::vector<llvm::Type*> paramTypes(params->mParams.size(), llvm::Type::getDoubleTy(*sContext));

	llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getDoublePtrTy(*sContext), paramTypes, false);
	llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, mName, sModule.get());

	// Create basic block to start insertion into
	llvm::BasicBlock* basicBlock = llvm::BasicBlock::Create(*sContext, "entry", func);
	sBuilder->SetInsertPoint(basicBlock);

	unsigned int index = 0;
	for (auto& arg : func->args()) {
		FunctionParamNode* param = dynamic_cast<FunctionParamNode*>(params->mParams[index]);
		// arg.setName(param->mName);
		llvm::AllocaInst* allocaInst = CreateEntryBlockAlloca(func, param->mName);
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