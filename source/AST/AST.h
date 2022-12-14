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

    TypeNode(Token typeClass, const std::string& identifier = "") : mTypeClass(typeClass), mIdentifier(identifier) {}
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

class RelationalExpressionNode: public ASTNode {
public:
    ASTNode* mLeft;
    ASTNode* mRight;
    Token mOperator;

    RelationalExpressionNode(ASTNode* left, ASTNode* right, Token op) : mLeft(left), mRight(right), mOperator(op) {}
    llvm::Value* Codegen() override;
};

class BinaryExpressionNode : public ASTNode {
public:
    ASTNode* mLeft;
    ASTNode* mRight;
    Token mOperator;

    BinaryExpressionNode(ASTNode* left, ASTNode* right, Token op) : mLeft(left), mRight(right), mOperator(op) {}
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

class AssignmentExpressionNode : public ASTNode {
public:
    std::string mIdentifier;
    ASTNode* mExpression;

    AssignmentExpressionNode(const std::string& id, ASTNode* expr) : mIdentifier(id), mExpression(expr) {}
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

// TODO: AST Node for function calls