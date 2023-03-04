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

struct NamedValueData {
	// TODO: This should probably store something better than the raw type node
	TypeNode* mType;
	llvm::AllocaInst* mAlloca;
};

static std::unique_ptr<llvm::LLVMContext> sContext;
static std::unique_ptr<llvm::Module> sModule;
static std::unique_ptr<llvm::IRBuilder<>> sBuilder;
static std::unordered_map<std::string, NamedValueData> sNamedValues;
static std::unordered_map<std::string, llvm::Function*> sFunctions;

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
		// TODO: Assert...
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
	if (!errorCode) {
		// TODO: Assert...
	}

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
	llvm::Type* GetRawLLVMType(TypeNode* type) {
		if (type->mTypeClass == Token::TYPE_I32) {
			return llvm::Type::getInt32Ty(*sContext);
		}
		if (type->mTypeClass == Token::TYPE_F32) {
			return llvm::Type::getFloatTy(*sContext);
		}
		if (type->mTypeClass == Token::TYPE_BOOL) {
			return llvm::Type::getInt1Ty(*sContext);
		}
		// TODO: Error?
		return nullptr;
	}

	llvm::Type* GetLLVMType(TypeNode* type) {
		llvm::Type* rawType = GetRawLLVMType(type);
		if (type->mIsArray) {
			if (type->mArraySize) {
				const int arraySize = std::stoi(type->mArraySize->mNumber);
				return llvm::ArrayType::get(rawType, arraySize);
			}
			else {
				return llvm::PointerType::getUnqual(rawType);
			}
		}
		return rawType;
	}

	llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* func, const std::string& varName, TypeNode* type) {
		llvm::IRBuilder<> tempBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
		return tempBuilder.CreateAlloca(GetLLVMType(type), 0, varName.c_str());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

llvm::Value* IdentifierNode::Codegen() {
	NamedValueData& valueData = sNamedValues[mIdentifier];
	llvm::AllocaInst* value = valueData.mAlloca;
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

llvm::Value* BinaryOperatorNode::Codegen() {
	llvm::Value* left = mLeft->Codegen();
	llvm::Value* right = mRight->Codegen();
	if (!left || !right) {
		// TODO: Error
		return nullptr;
	}
	// TODO: Might want to perform some form of type checking/casting here
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
	case Token::ASSIGNMENT: {
		// TODO: This should be differentiated in grammar so lvalue vs rvalue identifier nodes are handled differently
		return sBuilder->CreateStore(right, left);
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
		// TODO: Maybe this should depend on type?
		initVal = llvm::ConstantFP::get(*sContext, llvm::APFloat(0.0f));
	}
	llvm::AllocaInst* allocaInst = CreateEntryBlockAlloca(parent, mIdentifier, mType);
	sBuilder->CreateStore(initVal, allocaInst);

	sNamedValues[mIdentifier] = NamedValueData{ mType, allocaInst };
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
	// TODO: Need more validation here
	// Transform the parameter type list into LLVM types
	std::vector<llvm::Type*> paramTypes;
	if (mParamList) {
		std::transform(mParamList->mParams.cbegin(), mParamList->mParams.cend(), std::back_inserter(paramTypes), [](FunctionParamNode* rawParam) {
			return GetLLVMType(rawParam->mType);
		});
	}

	llvm::FunctionType* funcType = llvm::FunctionType::get(GetLLVMType(mType), paramTypes, false);
	llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, mName, sModule.get());

	// Create basic block to start insertion into
	llvm::BasicBlock* basicBlock = llvm::BasicBlock::Create(*sContext, "entry", func);
	sBuilder->SetInsertPoint(basicBlock);

	unsigned int index = 0;
	for (auto& arg : func->args()) {
		FunctionParamNode* param = mParamList->mParams[index];
		llvm::AllocaInst* allocaInst = CreateEntryBlockAlloca(func, param->mName, param->mType);
		sBuilder->CreateStore(&arg, allocaInst);
		sNamedValues[param->mName] = NamedValueData{ param->mType, allocaInst };
		index++;
	}

	if (llvm::Value* value = mBlockExpr->Codegen()) {
		sBuilder->CreateRet(value);

		llvm::verifyFunction(*func);
		// return func;
	}
	sFunctions[mName] = func;

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

llvm::Value* FunctionArgumentListNode::Codegen() {
	return nullptr;
}

llvm::Value* FunctionCallNode::Codegen() {
	// generate code for arguments first
	std::vector<llvm::Value*> arguments;
	if (mArgumentList) {
		for (ASTNode* argument : mArgumentList->mArguments) {
			arguments.push_back(argument->Codegen());
		}
	}

	llvm::Function* func = sFunctions[mFuncName];
	// TODO: Validate func name/arguments
	llvm::CallInst* call = sBuilder->CreateCall(func, arguments);
	return call;
}

llvm::Value* ArrayAccessNode::Codegen() {
	// This assumes the codegen is used to get the value of an array element
	// TODO: Better way to differentiate between lvalue/rvalue access of array
	NamedValueData& valueData = sNamedValues[mName];
	llvm::AllocaInst* value = valueData.mAlloca;
	if (!value) {
		// TODO: Error
		return nullptr;
	}
	else {
		// llvm::Type* type = GetLLVMType(valueData.mType);
		llvm::Type* elementType = GetRawLLVMType(valueData.mType);
		// elementType = type->getArrayElementType();
		// TODO: Figure out why this zero index is no longer required
		//	- I'm confused... probably something to do with opaque pointers?
		//  - I think it's probably because I changed the type being passed into GEP to element type instead of ptr/array type
		// llvm::ConstantInt* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*sContext), 0);
		llvm::Value* index = mArrayIndexExpr->Codegen();
		llvm::Value* indices[] = { index };
		llvm::Value* elementPtr = sBuilder->CreateGEP(elementType, value, indices);
		if (mIsMemLocation) {
			return elementPtr;
		}
		else {
			return sBuilder->CreateLoad(elementType, elementPtr, mName.c_str());
		}
	}
	return nullptr;
}