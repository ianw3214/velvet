#include <iostream>

#include <vector>

#include "common.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

int main() {
    // Lexer::LoadInputString("if something 135 then something else something");
    // Lexer::LoadInputString("a > test <> 100 = 1000");
    // Lexer::LoadInputString("( a > test ) <> ( 100 = 1000 )");
    // Lexer::LoadInputString("if a then if x then y else c");
    // Lexer::LoadInputString("if a then if x then y - 30 else c + 100");
    // Lexer::LoadInputString("var test $ type; test := a - b + 1000;");
    Lexer::LoadInputString("fn testfunc () -> type { var test $ type; test := a - b + 1000; if 10 + 10 then b else c };");

    ASTNode * base = Parser::Parse();

    // depth first traversal of AST
    std::vector<ASTNode*> stack;
    stack.push_back(base);
    while (!stack.empty()) {
        ASTNode * top = stack.back();
        stack.pop_back();
        if (IdentifierNode* id = dynamic_cast<IdentifierNode*>(top)) {
            std::cout << "VISITED ID NODE: " << id->mIdentifier << '\n';
        }
        if (NumberNode* num = dynamic_cast<NumberNode*>(top)) {
            std::cout << "VISITED NUMBER NODE: " << num->mNumber << '\n';
        }
        if (IfExpressionNode* ifExpr = dynamic_cast<IfExpressionNode*>(top)) {
            std::cout << "VISITED IF EXPR NODE\n";
            stack.push_back(ifExpr->mThenNode);
            stack.push_back(ifExpr->mElseNode);
            stack.push_back(ifExpr->mConditionNode);
        }
        if (RelationalOperatorNode* relOp = dynamic_cast<RelationalOperatorNode*>(top)) {
            std::cout << "VISITED REL OPERATOR NODE: " << "relOp->mOperator" << '\n';
            stack.push_back(relOp->mRight);
            stack.push_back(relOp->mLeft);
        }
        if (BinaryOperatorNode* binOp = dynamic_cast<BinaryOperatorNode*>(top)) {
            std::cout << "VISITED REL OPERATOR NODE: " << "binOp->mOperator" << '\n';
            stack.push_back(binOp->mRight);
            stack.push_back(binOp->mLeft);
        }
        if (AssignmentStatementNode* assignStmt = dynamic_cast<AssignmentStatementNode*>(top)) {
            std::cout << "VISITED ASSIGNMENT STATEMENT NODE: " << assignStmt->mIdentifier << '\n';
            stack.push_back(assignStmt->mExpression);
        }
        if (VariableDeclarationNode* declStmt = dynamic_cast<VariableDeclarationNode*>(top)) {
            std::cout << "VISITED VARIABLE DECLARATION NODE: " << declStmt->mIdentifier << "(type: " << declStmt->mType << ")\n";
            stack.push_back(declStmt->mExpression);
        }
        if (StatementListNode* stmtList = dynamic_cast<StatementListNode*>(top)) {
            std::cout << "VISITED STATEMENT LIST OF SIZE " << stmtList->mStatements.size() << '\n';
            for (ASTNode* node : stmtList->mStatements) {
                stack.push_back(node);
            }
        }
        if (FunctionDeclNode* funcDecl = dynamic_cast<FunctionDeclNode*>(top)) {
            std::cout << "VISITED FUNCTION NODE W/ NAME: " << funcDecl->mName << " AND TYPE: " << funcDecl->mType << '\n';
            stack.push_back(funcDecl->mBlockExpr);
        }
        if (BlockExpressionNode* blockExpr = dynamic_cast<BlockExpressionNode*>(top)) {
            std::cout << "VISITED BLOCK EXPRESSION NODE\n";
            stack.push_back(blockExpr->mStatementList);
            stack.push_back(blockExpr->mExprNode);
        }
    }

    initLLVM();
    base->Codegen();
    printLLVM();

    return 0;
};