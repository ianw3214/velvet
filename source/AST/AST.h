#pragma once

#include <string>
#include <vector>

#include "llvm/IR/Value.h"

#include "common.h"

// DEBUG ONLY
// TODO: REMOVE (refactor somewhere)
void initLLVM();
void printLLVM();

class ASTNode {
public:
    virtual ~ASTNode() {}
    virtual llvm::Value* Codegen() = 0;
};

class IdentifierNode : public ASTNode {
public:
    std::string mIdentifier;
    
    IdentifierNode(const std::string& identifier) : mIdentifier(identifier) {}
    llvm::Value* Codegen() override;
};

class NumberNode : public ASTNode {
public:
    std::string mNumber;

    NumberNode(const std::string& number) : mNumber(number) {}
    llvm::Value* Codegen() override;
};

class TypeNode : public ASTNode {
public:
    Token mTypeClass;
    std::string mIdentifier;
    // TODO: This will have to handle multidimensional array types
    bool mIsArray;
    NumberNode* mArraySize;

    TypeNode(Token typeClass, bool isArray = false, NumberNode* arraySize = nullptr, const std::string& identifier = "") : mTypeClass(typeClass), mIdentifier(identifier), mIsArray(isArray), mArraySize(arraySize) {}
    llvm::Value* Codegen() override;
};

class ExpressionListNode : public ASTNode {
public:
    std::vector<ASTNode*> mExpressions;

    ExpressionListNode(std::vector<ASTNode*>&& expressions) : mExpressions(expressions) {}
    llvm::Value* Codegen() override;
};

class BlockExpressionNode : public ASTNode {
public:
    ExpressionListNode* mExpressionList;

    BlockExpressionNode(ExpressionListNode* expressionList) : mExpressionList(expressionList) {}
    llvm::Value* Codegen() override;
};

class IfExpressionNode : public ASTNode {
public:
    ASTNode* mConditionNode;
    ASTNode* mThenNode;
    ASTNode* mElseNode;

    IfExpressionNode(ASTNode* cond, ASTNode* then, ASTNode* els) : mConditionNode(cond), mThenNode(then), mElseNode(els) {}
    llvm::Value* Codegen() override;
};

class LoopExpressionNode : public ASTNode {
public:
    BlockExpressionNode* mBlockNode;

    LoopExpressionNode(BlockExpressionNode* block) : mBlockNode(block) {}
    llvm::Value* Codegen() override;
};

class BinaryOperatorNode : public ASTNode {
public:
    ASTNode* mLeft;
    ASTNode* mRight;
    Token mOperator;

    BinaryOperatorNode(ASTNode* left, ASTNode* right, Token op) : mLeft(left), mRight(right), mOperator(op) {}
    llvm::Value* Codegen() override;
};

class VariableDeclarationNode : public ASTNode {
public:
    std::string mIdentifier;
    TypeNode* mType;
    ASTNode* mExpression;

    VariableDeclarationNode(const std::string id, TypeNode* type, ASTNode* expr) : mIdentifier(id), mType(type), mExpression(expr) {}
    llvm::Value* Codegen() override;
};

class FunctionParamNode : public ASTNode {
public:
    std::string mName;
    TypeNode* mType;

    FunctionParamNode(const std::string& id, TypeNode* type) : mName(id), mType(type) {}
    llvm::Value* Codegen() override;
};

class FunctionParamListNode : public ASTNode {
public:
    std::vector<FunctionParamNode*> mParams;

    FunctionParamListNode(std::vector<FunctionParamNode*>&& params) : mParams(params) {}
    llvm::Value* Codegen() override;
};

class FunctionDeclNode : public ASTNode {
public:
    std::string mName;
    TypeNode* mType;
    FunctionParamListNode* mParamList;
    BlockExpressionNode* mBlockExpr;

    FunctionDeclNode(const std::string& name, TypeNode* type, FunctionParamListNode* paramList, BlockExpressionNode* blockExpr) : mName(name), mType(type), mParamList(paramList), mBlockExpr(blockExpr) {}
    llvm::Value* Codegen() override;
};

class FunctionArgumentListNode : public ASTNode {
public:
    std::vector<ASTNode*> mArguments;

    FunctionArgumentListNode(std::vector<ASTNode*>&& arguments) : mArguments(arguments) {}
    llvm::Value* Codegen() override;
};

class FunctionCallNode : public ASTNode {
public:
    std::string mFuncName;
    FunctionArgumentListNode* mArgumentList;

    FunctionCallNode(const std::string& funcName, FunctionArgumentListNode* argumentList) : mFuncName(funcName), mArgumentList(argumentList) {}
    llvm::Value* Codegen() override;
};

class ArrayAccessNode : public ASTNode {
public:
    std::string mName;
    ASTNode* mArrayIndexExpr;

    bool mIsMemLocation;

    ArrayAccessNode(const std::string& name, ASTNode* arrayIndexExpr, bool isMemLocation = false) : mName(name), mArrayIndexExpr(arrayIndexExpr), mIsMemLocation(isMemLocation) {}
    llvm::Value* Codegen() override;
};