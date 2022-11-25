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

class BlockExpressionNode : public ASTNode {
public:
    ASTNode* mExpressionList;

    BlockExpressionNode(ASTNode* expressionList) : mExpressionList(expressionList) {}
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
    ASTNode* mBlockNode;

    LoopExpressionNode(ASTNode* block) : mBlockNode(block) {}
    llvm::Value* Codegen() override;
};

class RelationalOperatorNode : public ASTNode {
public:
    ASTNode* mLeft;
    ASTNode* mRight;
    Token mOperator;

    RelationalOperatorNode(ASTNode* left, ASTNode* right, Token op) : mLeft(left), mRight(right), mOperator(op) {}
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
    std::string mType;
    ASTNode* mExpression;

    VariableDeclarationNode(const std::string id, const std::string type, ASTNode* expr) : mIdentifier(id), mType(type), mExpression(expr) {}
    llvm::Value* Codegen() override;
};

class AssignmentExpressionNode : public ASTNode {
public:
    std::string mIdentifier;
    ASTNode* mExpression;

    AssignmentExpressionNode(const std::string& id, ASTNode* expr) : mIdentifier(id), mExpression(expr) {}
    llvm::Value* Codegen() override;
};

class ExpressionListNode : public ASTNode {
public:
    std::vector<ASTNode*> mExpressions;

    ExpressionListNode(std::vector<ASTNode*>&& expressions) : mExpressions(expressions) {}
    llvm::Value* Codegen() override;
};

class FunctionDeclNode : public ASTNode {
public:
    std::string mName;
    std::string mType;
    ASTNode* mBlockExpr;

    FunctionDeclNode(const std::string& name, const std::string& type, ASTNode* blockExpr) : mName(name), mType(type), mBlockExpr(blockExpr) {}
    llvm::Value* Codegen() override;
};

// TODO: AST Node for function calls