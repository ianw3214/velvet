#pragma once

#include <string>

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

class IfExpressionNode : public ASTNode {
public:
    ASTNode* mConditionNode;
    ASTNode* mThenNode;
    ASTNode* mElseNode;

    IfExpressionNode(ASTNode* cond, ASTNode* then, ASTNode* els) : mConditionNode(cond), mThenNode(then), mElseNode(els) {}
};

class BinaryOperatorNode : public ASTNode {
public:
    ASTNode* mLeft;
    ASTNode* mRight;
    Token mOperator;

    BinaryOperatorNode(ASTNode* left, ASTNode* right, Token op) : mLeft(left), mRight(right), mOperator(op) {}
};