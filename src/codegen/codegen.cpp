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
    , mNamedValues() 
    , mNamedVariables()
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
    mErrorHandler.logError("No valid expression was generated");
    return nullptr;
}

llvm::Function* CodeGenerator::generateFunctionCode(FunctionDefinitionNode& functionDefinition) {
    std::vector<llvm::Type*> argumentTypes = std::vector<llvm::Type*>();
    argumentTypes.reserve(functionDefinition.mArguments.size());
    for (auto& argument : functionDefinition.mArguments) {
        llvm::Type* type = _getRawLLVMType(argument.second);
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
    size_t index = 0;
    for (auto& argument : func->args()) {
        const std::string& argName = functionDefinition.mArguments[index].first;
        argument.setName(argName);
        mNamedValues[argName] = &argument;
        index++;
    }
    llvm::BasicBlock* basicBlock = llvm::BasicBlock::Create(*mContext, "entry", func);
    mBuilder->SetInsertPoint(basicBlock);
    llvm::Value* returnValue = generateExpressionCode(functionDefinition.mExpression);
    if (!returnValue) {
        // Maybe this is not an error? fix this if it turns out to be the case
        mErrorHandler.logError("No return value was generated for function expression");
        return nullptr;
    }
    mBuilder->CreateRet(returnValue);
    llvm::verifyFunction(*func);
    return func;
}

std::unique_ptr<llvm::Module>& CodeGenerator::getModule() {
    return mModule;
}

llvm::Value* CodeGenerator::_generateVariableAccess(std::unique_ptr<VariableAccessNode>& varAccess) {
    const std::string& varName = varAccess->mName.mIdentifier;
    {   // values
        // TODO: Convert parameter values to alloca so it can be array typed as well
        auto it = mNamedValues.find(varName);
        if (it != mNamedValues.end()) {
            return it->second;
        }
    }
    {   // variables
        auto it = mNamedVariables.find(varName);
        if (it != mNamedVariables.end()) {
            llvm::AllocaInst* alloca = it->second;
            if (varAccess->mArrayIndex.has_value()) {
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
            else {
                return mBuilder->CreateLoad(alloca->getAllocatedType(), alloca, varName);
            }
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
    // Need to deal with return statements and such...
    return last;   
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
        if (!thenValue) {
            mErrorHandler.logError("No then value was generated for conditional expression");
            return nullptr;
        }
        mBuilder->CreateBr(mergeBlock);
        mBuilder->SetInsertPoint(elseBlock);
        llvm::Value* elseValue = generateExpressionCode(conditional->mElse.value());
        if (!elseValue) {
            mErrorHandler.logError("No else value was generated for conditional expression");
            return nullptr;
        }
        mBuilder->CreateBr(mergeBlock);
        // TODO: Handle then/else basic blocks changing the actual basic block
        //  - need to call builder::getInsertBlock to use the newest insert block as then/else
        mBuilder->SetInsertPoint(mergeBlock);
        llvm::PHINode* phi = mBuilder->CreatePHI(llvm::Type::getDoubleTy(*mContext), 2, "condExprVal");
        phi->addIncoming(thenValue, thenBlock);
        phi->addIncoming(elseValue, elseBlock);
        return phi;
    }
    else {
        mBuilder->CreateCondBr(conditionValue, thenBlock, mergeBlock);
        mBuilder->SetInsertPoint(thenBlock);
        llvm::Value* thenValue = generateExpressionCode(conditional->mThen);
        if (!thenValue) {
            mErrorHandler.logError("No then value was generated for conditional expression");
            return nullptr;
        }
        mBuilder->CreateBr(mergeBlock);
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
    mNamedVariables[varName] = alloca;
    if (varDef->mInitialValue.has_value()) {
        llvm::Value* value = generateExpressionCode(varDef->mInitialValue.value());
        mBuilder->CreateStore(value, alloca);
    }
    return nullptr;
}

llvm::Value* CodeGenerator::_generateAssignment(std::unique_ptr<AssignmentNode>& assignment) {
    VariableAccessNode& varAccess = assignment->mVariable.mVariable;
    const std::string& varName = varAccess.mName.mIdentifier;
    llvm::Value* memLocation = nullptr;
    // shares a lot of code with VariableAccess, perhaps can refactor somehow
    {   // values
        // TODO: Convert parameter values to alloca so it can be array typed as well
        auto it = mNamedValues.find(varName);
        if (it != mNamedValues.end()) {
            memLocation = it->second;
        }
    }
    {   // variables
        auto it = mNamedVariables.find(varName);
        if (it != mNamedVariables.end()) {
            llvm::AllocaInst* alloca = it->second;
            if (varAccess.mArrayIndex.has_value()) {
                llvm::Type* type = alloca->getAllocatedType();
                llvm::Type* elementType = type->getArrayElementType();
                llvm::ConstantInt* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*mContext), 0);
                ExpressionNodeOwner& expr = varAccess.mArrayIndex.value();
                llvm::Value* indexExpr = generateExpressionCode(expr);
                llvm::Value* indices[] = { zero, indexExpr };
                memLocation = mBuilder->CreateGEP(type, alloca, indices);
            }
            else {
                memLocation = mBuilder->CreateLoad(alloca->getType(), alloca, varName);
            }
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