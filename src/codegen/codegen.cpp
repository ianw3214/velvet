#include "codegen.h"

#include "llvm/IR/Verifier.h"

#include <iostream>

llvm::Type* CodeGenerator::_getRawLLVMType(Token type) const {
    if (type == Token::TYPE_I32) {
        return llvm::Type::getInt32Ty(*mContext);
    }
    if (type == Token::TYPE_F32) {
        return llvm::Type::getFloatTy(*mContext);
    }
    if (type == Token::TYPE_BOOL) {
        return llvm::Type::getInt1Ty(*mContext);
    }
    mErrorHandler.logError("No corresponding LLVM type could be found");
    return nullptr;
}

CodeGenerator::CodeGenerator(ErrorHandler& handler) 
    : mContext(std::make_unique<llvm::LLVMContext>())
    , mModule(std::make_unique<llvm::Module>("velvet", *mContext))
    , mBuilder(std::make_unique<llvm::IRBuilder<>>(*mContext)) 
    , mErrorHandler(handler)
    , mNamedVariables()
    , mFunctions()
    , mLoopStack() 
{
    if (!mContext) {
        mErrorHandler.logError("Could not initialize LLVM context");
        return;
    }
    if (!mModule) {
        mErrorHandler.logError("Could not initialize LLVM module");
        return;
    }
    if (!mBuilder) {
        mErrorHandler.logError("Could not initialize LLVM builder");
        return;
    }

    setupDefaultFunctions();
}

void CodeGenerator::setupDefaultFunctions() {
    llvm::FunctionType* printfType = llvm::FunctionType::get(
        llvm::IntegerType::getInt32Ty(*mContext),
        llvm::PointerType::get(llvm::Type::getInt8Ty(*mContext), 0), true);
    mFunctions["printf"] = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", *mModule);
}

// can the passed in expression owner be const ref?
llvm::Value* CodeGenerator::generateExpressionCode(ExpressionNodeOwner& expressionNode) {
    if (auto variable = std::get_if<std::unique_ptr<VariableAccessNode>>(&expressionNode)) {
        return _generateVariableAccess(*variable);
    }
    if (auto number = std::get_if<std::unique_ptr<NumberNode>>(&expressionNode)) {
        return _generateNumber(*number);
    }
    if (auto scope = std::get_if<std::unique_ptr<ScopeNode>>(&expressionNode)) {
        return _generateScope(*scope); 
    }
    if (auto arrayValue = std::get_if<std::unique_ptr<ArrayValueNode>>(&expressionNode)) {
        // This is actually not implemented, needs special case to codegen
        return nullptr;
    }
    if (auto conditional = std::get_if<std::unique_ptr<ConditionalNode>>(&expressionNode)) {
        return _generateConditional(*conditional);
    }
    if (auto binop = std::get_if<std::unique_ptr<BinaryOperationNode>>(&expressionNode)) {
        return _generateBinaryOperation(*binop);
    }
    if (auto vardef = std::get_if<std::unique_ptr<VariableDefinitionNode>>(&expressionNode)) {
        return _generateVariableDefinition(*vardef);
    }
    if (auto assign = std::get_if<std::unique_ptr<AssignmentNode>>(&expressionNode)) {
        return _generateAssignment(*assign);
    }
    if (auto loop = std::get_if<std::unique_ptr<LoopNode>>(&expressionNode)) {
        return _generateLoop(*loop);
    }
    if (auto br = std::get_if<std::unique_ptr<BreakNode>>(&expressionNode)) {
        return _generateBreak(*br);
    }
    mErrorHandler.logError("No valid expression was generated");
    return nullptr;
}

llvm::Function* CodeGenerator::generateFunctionCode(FunctionDefinitionNode& functionDefinition) {
    if (mFunctions.find(functionDefinition.mName.mIdentifier) != mFunctions.end()) {
        mErrorHandler.logError("Function already exists");
        return nullptr;
    }
    std::vector<llvm::Type*> argumentTypes = std::vector<llvm::Type*>();
    argumentTypes.reserve(functionDefinition.mArguments.size());
    for (auto& argument : functionDefinition.mArguments) {
        llvm::Type* type = _getRawLLVMType(argument.second.mRawType);
        if (argument.second.mArraySize >= 0) {
            if (argument.second.mIsArrayDecay) {
                type = llvm::PointerType::getUnqual(*mContext);
            }
            else {
                type = llvm::ArrayType::get(type, argument.second.mArraySize);
            }
        }
        if (!type) {
            mErrorHandler.logError("Invalid type handled in function argument");
            return nullptr;
        }
        argumentTypes.push_back(type);
    }
    llvm::Type* returnType = _getRawLLVMType(functionDefinition.mReturnType);
    if (!returnType) {
        mErrorHandler.logError("Invalid type handled in function return type");
        return nullptr;
    }
    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, argumentTypes, false);
    llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, functionDefinition.mName.mIdentifier, *mModule);
    if (func == nullptr) {
        mErrorHandler.logError("Could not generate function");
        return nullptr;
    }
    llvm::BasicBlock* basicBlock = llvm::BasicBlock::Create(*mContext, "entry", func);
    mBuilder->SetInsertPoint(basicBlock);
    size_t index = 0;
    for (auto& argument : func->args()) {
        const auto& argumentDefinition = functionDefinition.mArguments[index]; 
        argument.setName(argumentDefinition.first);
        llvm::Type* type = _getRawLLVMType(argumentDefinition.second.mRawType);
        if (argumentDefinition.second.mArraySize >= 0) {
            if (argumentDefinition.second.mIsArrayDecay) {
                type = llvm::PointerType::getUnqual(*mContext);
            }
            else {
                type = llvm::ArrayType::get(type, argumentDefinition.second.mArraySize);
            }
        }
        llvm::AllocaInst* alloca = mBuilder->CreateAlloca(type, nullptr, argumentDefinition.first);
        mBuilder->CreateStore(&argument, alloca);
        mNamedVariables[argumentDefinition.first] = { 
            alloca, 
            argumentDefinition.second.mRawType,
            argumentDefinition.second.mIsArrayDecay };
        index++;
    }
    llvm::Value* returnValue = generateExpressionCode(functionDefinition.mExpression);
    if (!returnValue) {
        // Maybe this is not an error? fix this if it turns out to be the case
        mErrorHandler.logError("No return value was generated for function expression");
        return nullptr;
    }
    mBuilder->CreateRet(returnValue);
    llvm::verifyFunction(*func);
    mFunctions[functionDefinition.mName.mIdentifier] = func;
    return func;
}

std::unique_ptr<llvm::Module>& CodeGenerator::getModule() {
    return mModule;
}

llvm::Value* CodeGenerator::_generateVariableAccess(std::unique_ptr<VariableAccessNode>& varAccess) {
    const std::string& varName = varAccess->mName.mIdentifier;
    auto it = mNamedVariables.find(varName);
    if (it != mNamedVariables.end()) {
        llvm::AllocaInst* alloca = it->second.mAlloca;
        if (varAccess->mArrayIndex.has_value()) {
            if (it->second.mIsDecayedArray) {
                llvm::Value* ptrAddr = mBuilder->CreateLoad(llvm::PointerType::getUnqual(*mContext), alloca, "ptrload");
                llvm::Type* type = _getRawLLVMType(it->second.mRawType);
                ExpressionNodeOwner& expr = varAccess->mArrayIndex.value();
                llvm::Value* indexExpr = generateExpressionCode(expr);
                llvm::Value* elementPtr = mBuilder->CreateGEP(type, ptrAddr, { indexExpr });
                return mBuilder->CreateLoad(type, elementPtr, varName.c_str());
            }
            else {
                llvm::Type* type = alloca->getAllocatedType();
                llvm::Type* elementType = type->getArrayElementType();
                llvm::ConstantInt* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*mContext), 0);
                ExpressionNodeOwner& expr = varAccess->mArrayIndex.value();
                llvm::Value* indexExpr = generateExpressionCode(expr);
                llvm::Value* indices[] = { zero, indexExpr };
                llvm::Value* elementPtr = mBuilder->CreateGEP(type, alloca, indices);
                // TODO: Handle if var access is used as lvalue
                return mBuilder->CreateLoad(elementType, elementPtr, varName.c_str());
            }
        }
        else {
            if (varAccess->mArrayDecay) {
                llvm::ConstantInt* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*mContext), 0);
                llvm::Value* indices[] = { zero, zero };
                return mBuilder->CreateGEP(alloca->getAllocatedType(), alloca, indices, "arrdecay");
            }
            else {
                return mBuilder->CreateLoad(alloca->getAllocatedType(), alloca, varName);
            }
        }
    }
    auto funcIt = mFunctions.find(varName);
    if (funcIt != mFunctions.end()) {
        // Special cases
        if (funcIt->first == "printf") {
            if (varAccess->mCallArgs.has_value()) {
                std::vector<ExpressionNodeOwner>& argExpressions = varAccess->mCallArgs.value();
                if (argExpressions.empty()) {
                    mErrorHandler.logError("Print statement should always have an argument");
                    return nullptr;
                }
                llvm::Value* formatString = nullptr;
                llvm::Value* value = generateExpressionCode(argExpressions.front());
                if (value->getType()->isFloatTy()) {
                    formatString = mBuilder->CreateGlobalStringPtr("%f\n", "formatStringf");
                    // float needs to be promoted to a double to work with printf
                    value = mBuilder->CreateFPExt(value, llvm::Type::getDoubleTy(*mContext));
                } else {
                    formatString = mBuilder->CreateGlobalStringPtr("%d\n", "formatStringd");
                }
                llvm::Value* printfArgs[] = { formatString, value };
                return mBuilder->CreateCall(mFunctions["printf"], printfArgs, "calltmp");
            }
            else {
                mErrorHandler.logError("Print statement should always have an argument");
                return nullptr;
            }
        }
        if (varAccess->mCallArgs.has_value()) {
            std::vector<ExpressionNodeOwner>& argExpressions = varAccess->mCallArgs.value();
            auto it = mFunctions.find(varName);
            if (it == mFunctions.end()) {
                mErrorHandler.logError("Could not find function");
                return nullptr;
            }
            if (it->second->arg_size() != argExpressions.size()) {
                mErrorHandler.logError("Mismatched number of function arguments");
                return nullptr;
            }
            std::vector<llvm::Value*> values;
            for (ExpressionNodeOwner& expr : argExpressions) {
                values.emplace_back(generateExpressionCode(expr));
            }
            return mBuilder->CreateCall(it->second, values, "calltmp");
        }
        else {
            mErrorHandler.logError("Expected argument list after function call");
            return nullptr;
        }
    }
    mErrorHandler.logError("Could not find existing symbol for identifier");
    return nullptr;
}

llvm::Value* CodeGenerator::_generateNumber(std::unique_ptr<NumberNode>& number) {
    if (float* num = std::get_if<float>(&number->mNumber)) {
        return llvm::ConstantFP::get(*mContext, llvm::APFloat(*num));
    }
    if (int* num = std::get_if<int>(&number->mNumber)) {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*mContext), *num);
    }
    mErrorHandler.logError("Something went wrong while parsing a number");
    return nullptr;
}

llvm::Value* CodeGenerator::_generateScope(std::unique_ptr<ScopeNode>& scope) {
    llvm::Value* last = nullptr;
    for (ExpressionNodeOwner& expression : scope->mExpressionList) {
        last = generateExpressionCode(expression);
    }
    // TODO: Need to deal with return statements and such...
    return last;   
}

llvm::Value* CodeGenerator::_generateArrayValue(std::unique_ptr<ArrayValueNode>& arrayValue, llvm::AllocaInst* alloca) {
    int index = 0;
    for (ExpressionNodeOwner& expr : arrayValue->mExpressionList) {
        // TODO: This pattern seems to be used a lot, might want to refactor it out
        llvm::Type* type = alloca->getAllocatedType();
        llvm::Type* elementType = type->getArrayElementType();
        llvm::ConstantInt* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*mContext), 0);
        llvm::Value* indexExpr = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*mContext), index);
        llvm::Value* indices[] = { zero, indexExpr };
        llvm::Value* memLocation = mBuilder->CreateGEP(type, alloca, indices);

        llvm::Value* valueExpr = generateExpressionCode(expr);
        mBuilder->CreateStore(valueExpr, memLocation);
        index++;
    }
    // we don't actually need to return a value since we should already be assigning to the alloca
    return nullptr;
}

llvm::Value* CodeGenerator::_generateConditional(std::unique_ptr<ConditionalNode>& conditional) {
    llvm::Function* parentFunc = mBuilder->GetInsertBlock()->getParent();
    llvm::Value* conditionValue = generateExpressionCode(conditional->mCondition);
    // we may also want to check that the condition value is a valid boolean typed expression
    if (!conditionValue) {
        mErrorHandler.logError("No conditional value was generated for conditional expression");
        return nullptr;
    }

    llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(*mContext, "then", parentFunc);
    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*mContext, "merge", parentFunc);
    if (conditional->mElse.has_value()) {
        llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(*mContext, "else", parentFunc, mergeBlock);
        mBuilder->CreateCondBr(conditionValue, thenBlock, elseBlock);
        mBuilder->SetInsertPoint(thenBlock);
        llvm::Value* thenValue = generateExpressionCode(conditional->mThen);
        mBuilder->CreateBr(mergeBlock);
        mBuilder->SetInsertPoint(elseBlock);
        llvm::Value* elseValue = generateExpressionCode(conditional->mElse.value());
        mBuilder->CreateBr(mergeBlock);
        // TODO: Handle then/else basic blocks changing the actual basic block
        //  - need to call builder::getInsertBlock to use the newest insert block as then/else
        mBuilder->SetInsertPoint(mergeBlock);
        // if statements don't necessarily return a value, only do so if both then/else have return values
        if (thenValue && elseValue) {
            llvm::PHINode* phi = mBuilder->CreatePHI(llvm::Type::getDoubleTy(*mContext), 2, "condExprVal");
            phi->addIncoming(thenValue, thenBlock);
            phi->addIncoming(elseValue, elseBlock);
            return phi;
        }
        else {
            return nullptr;
        }
    }
    else {
        mBuilder->CreateCondBr(conditionValue, thenBlock, mergeBlock);
        mBuilder->SetInsertPoint(thenBlock);
        llvm::Value* _thenValue = generateExpressionCode(conditional->mThen);
        // If then block is a branch/return we want to not generate this
        llvm::Instruction* last = thenBlock->getTerminator();
        if (last && last->getOpcode() != llvm::Instruction::Br) {
            mBuilder->CreateBr(mergeBlock);
        }
        mBuilder->SetInsertPoint(mergeBlock);
        return nullptr;
    }
}

llvm::Value* CodeGenerator::_generateBinaryOperation(std::unique_ptr<BinaryOperationNode>& binaryOperation) {
    llvm::Value* left = generateExpressionCode(binaryOperation->mLeft);
    llvm::Value* right = generateExpressionCode(binaryOperation->mRight);
    if (!left || !right) {
        //  - might not need to error if we assume this is caught during expr codegen
        mErrorHandler.logError("Unexpected valueless expression in binary operation");
        return nullptr;
    }
    llvm::Type* operationType = left->getType();
    if (operationType != right->getType()) {
        mErrorHandler.logError("Mismatched types in binary operation");
        return nullptr;
    }
    switch(binaryOperation->mOperation) {
        case Token::PLUS: {
            if (operationType->isFloatingPointTy()) {
                return mBuilder->CreateFAdd(left, right, "addtmp");
            }
            else {
                return mBuilder->CreateAdd(left, right, "addtmp");
            }
        } break;
        case Token::MINUS: {
            if (operationType->isFloatingPointTy()) {
                return mBuilder->CreateFSub(left, right, "subtmp");
            }
            else {
                return mBuilder->CreateSub(left, right, "subtmp");
            }
        } break;
        case Token::MULTIPLY: {
            if (operationType->isFloatingPointTy()) {
                return mBuilder->CreateFMul(left, right, "multmp");
            }
            else {
                return mBuilder->CreateMul(left, right, "multmp");
            }
        } break;
        case Token::DIVIDE: {
            if (operationType->isFloatingPointTy()) {
                return mBuilder->CreateFDiv(left, right, "divtmp");
            }
            else {
                // SDiv vs UDIV
                return mBuilder->CreateSDiv(left, right, "divtmp");
            }
        } break;
        case Token::EQUALS: {
            if (operationType->isFloatingPointTy()) {
                return mBuilder->CreateFCmp(llvm::FCmpInst::FCMP_OEQ, left, right, "eqtmp");
            }
            else {
                return mBuilder->CreateICmp(llvm::ICmpInst::ICMP_EQ, left, right, "eqtmp");
            }
        } break;
        case Token::NOT_EQUALS: {
            if (operationType->isFloatingPointTy()) {
                return mBuilder->CreateFCmp(llvm::FCmpInst::FCMP_ONE, left, right, "neqtmp");
            }
            else {
                return mBuilder->CreateICmp(llvm::ICmpInst::ICMP_NE, left, right, "neqtmp");
            }
        } break;
        case Token::GREATER: {
            if (operationType->isFloatingPointTy()) {
                return mBuilder->CreateFCmp(llvm::FCmpInst::FCMP_OGT, left, right, "gttmp");
            }
            else {
                // SGT vs UGT
                return mBuilder->CreateICmp(llvm::ICmpInst::ICMP_SGT, left, right, "gttmp");
            }
        } break;
        case Token::GREATER_EQUALS: {
            if (operationType->isFloatingPointTy()) {
                return mBuilder->CreateFCmp(llvm::FCmpInst::FCMP_OGE, left, right, "geqtmp");
            }
            else {
                // SGT vs UGT
                return mBuilder->CreateICmp(llvm::ICmpInst::ICMP_SGE, left, right, "geqtmp");
            }
        } break;
        case Token::LESS: {
            if (operationType->isFloatingPointTy()) {
                return mBuilder->CreateFCmp(llvm::FCmpInst::FCMP_OLT, left, right, "lesstmp");
            }
            else {
                // SGT vs UGT
                return mBuilder->CreateICmp(llvm::ICmpInst::ICMP_SLT, left, right, "lesstmp");
            }
        } break;
        case Token::LESS_EQUALS: {
            if (operationType->isFloatingPointTy()) {
                return mBuilder->CreateFCmp(llvm::FCmpInst::FCMP_OLE, left, right, "leqtmp");
            }
            else {
                // SGT vs UGT
                return mBuilder->CreateICmp(llvm::ICmpInst::ICMP_SLE, left, right, "leqtmp");
            }
        } break;
        default: {
            mErrorHandler.logError("Unhandled binary operator");
        } break;
    }
    return nullptr;
}

llvm::Value* CodeGenerator::_generateVariableDefinition(std::unique_ptr<VariableDefinitionNode>& varDef) {
    llvm::Function* parentFunc = mBuilder->GetInsertBlock()->getParent();
    const std::string& varName = varDef->mName.mIdentifier;
    llvm::Type* varType = _getRawLLVMType(varDef->mType);
    if (varDef->mArraySize >= 0) {
        varType = llvm::ArrayType::get(varType, varDef->mArraySize);
    }
    llvm::AllocaInst* alloca = mBuilder->CreateAlloca(varType, nullptr, varName);
    mNamedVariables[varName] = { alloca, varDef->mType, false };
    if (varDef->mInitialValue.has_value()) {
        ExpressionNodeOwner& expr = varDef->mInitialValue.value();
        if (auto arrayValue = std::get_if<std::unique_ptr<ArrayValueNode>>(&expr)) {
            // I think the alloca needs to be created in here so it can handle alloca size as well
            _generateArrayValue(*arrayValue, alloca);
        }
        else {
            llvm::Value* value = generateExpressionCode(expr);
            mBuilder->CreateStore(value, alloca);
        }
    }
    return nullptr;
}

llvm::Value* CodeGenerator::_generateAssignment(std::unique_ptr<AssignmentNode>& assignment) {
    VariableAccessNode& varAccess = assignment->mVariable.mVariable;
    const std::string& varName = varAccess.mName.mIdentifier;
    llvm::Value* memLocation = nullptr;
    // shares a lot of code with VariableAccess, perhaps can refactor somehow
    auto it = mNamedVariables.find(varName);
    if (it != mNamedVariables.end()) {
        llvm::AllocaInst* alloca = it->second.mAlloca;
        if (varAccess.mArrayIndex.has_value()) {
            if (it->second.mIsDecayedArray) {
                llvm::Value* ptrAddr = mBuilder->CreateLoad(llvm::PointerType::getUnqual(*mContext), alloca, "ptrload");
                llvm::Type* type = _getRawLLVMType(it->second.mRawType);
                ExpressionNodeOwner& expr = varAccess.mArrayIndex.value();
                llvm::Value* indexExpr = generateExpressionCode(expr);
                memLocation = mBuilder->CreateGEP(type, ptrAddr, { indexExpr });
            }
            else {
                llvm::Type* type = alloca->getAllocatedType();
                llvm::Type* elementType = type->getArrayElementType();
                llvm::ConstantInt* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*mContext), 0);
                ExpressionNodeOwner& expr = varAccess.mArrayIndex.value();
                llvm::Value* indexExpr = generateExpressionCode(expr);
                llvm::Value* indices[] = { zero, indexExpr };
                memLocation = mBuilder->CreateGEP(type, alloca, indices);
            }
        }
        else {
            memLocation = alloca;
        }
    }
    if (!memLocation) {
        mErrorHandler.logError("No memory location found for assignment");
        return nullptr;
    }
    llvm::Value* value = generateExpressionCode(assignment->mValue);
    mBuilder->CreateStore(value, memLocation);
    return nullptr;
}

llvm::Value* CodeGenerator::_generateLoop(std::unique_ptr<LoopNode>& loop) {
    llvm::Function* parentFunc = mBuilder->GetInsertBlock()->getParent();
    llvm::BasicBlock* loopBlock = llvm::BasicBlock::Create(*mContext, "loop", parentFunc);
    llvm::BasicBlock* afterBlock = llvm::BasicBlock::Create(*mContext, "after");
    mBuilder->CreateBr(loopBlock);
    mBuilder->SetInsertPoint(loopBlock);

    mLoopStack.emplace_back(loopBlock, afterBlock);
    for (ExpressionNodeOwner& expression : loop->mExpressionList) {
        generateExpressionCode(expression);
    }
    mBuilder->CreateBr(loopBlock);
    afterBlock->insertInto(parentFunc);
    mBuilder->SetInsertPoint(afterBlock);
    mLoopStack.pop_back();
    return nullptr;
}

llvm::Value* CodeGenerator::_generateBreak(std::unique_ptr<BreakNode>& br) {
    if (mLoopStack.empty()) {
        mErrorHandler.logError("Cannot break if there is no loop");
        return nullptr;
    }
    mBuilder->CreateBr(mLoopStack.back().second);
    return nullptr;
}