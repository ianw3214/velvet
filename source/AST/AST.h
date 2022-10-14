#pragma once

#include <string>
#include <vector>

class ASTNode {
public:
    virtual ~ASTNode() {}
};

class IdentifierNode : public ASTNode {
public:
    std::string mIdentifier;
    
    IdentifierNode(const std::string& identifier) : mIdentifier(identifier) {}
};

class NumberNode : public ASTNode {
public:
    std::string mNumber;

    NumberNode(const std::string& number) : mNumber(number) {}
};

class BlockExpressionNode : public ASTNode {
public:
    ASTNode* mStatementList;
    ASTNode* mExprNode;

    BlockExpressionNode(ASTNode* statementList, ASTNode* expr) : mStatementList(statementList), mExprNode(expr) {}
};

class IfExpressionNode : public ASTNode {
public:
    ASTNode* mConditionNode;
    ASTNode* mThenNode;
    ASTNode* mElseNode;

    IfExpressionNode(ASTNode* cond, ASTNode* then, ASTNode* els) : mConditionNode(cond), mThenNode(then), mElseNode(els) {}
};

class RelationalOperatorNode : public ASTNode {
public:
    ASTNode* mLeft;
    ASTNode* mRight;
    Token mOperator;

    RelationalOperatorNode(ASTNode* left, ASTNode* right, Token op) : mLeft(left), mRight(right), mOperator(op) {}
};

class BinaryOperatorNode : public ASTNode {
public:
    ASTNode* mLeft;
    ASTNode* mRight;
    Token mOperator;

    BinaryOperatorNode(ASTNode* left, ASTNode* right, Token op) : mLeft(left), mRight(right), mOperator(op) {}
};

class VariableDeclarationNode : public ASTNode {
public:
    std::string mIdentifier;
    std::string mType;
    ASTNode* mExpression;

    VariableDeclarationNode(const std::string id, const std::string type, ASTNode* expr) : mIdentifier(id), mType(type), mExpression(expr) {}
};

class AssignmentStatementNode : public ASTNode {
public:
    std::string mIdentifier;
    ASTNode* mExpression;

    AssignmentStatementNode(const std::string& id, ASTNode* expr) : mIdentifier(id), mExpression(expr) {}
};

class StatementListNode : public ASTNode {
public:
    std::vector<ASTNode*> mStatements;

    StatementListNode(std::vector<ASTNode*>&& statements) : mStatements(statements) {}
};

class FunctionDeclNode : public ASTNode {
public:
    std::string mName;
    std::string mType;
    ASTNode* mBlockExpr;

    FunctionDeclNode(const std::string& name, const std::string& type, ASTNode* blockExpr) : mName(name), mType(type), mBlockExpr(blockExpr) {}
};