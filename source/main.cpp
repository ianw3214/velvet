#include <iostream>
#include "llvm/IR/LLVMContext.h"

#include <vector>

#include "common.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

int main() {
    llvm::LLVMContext context;
    std::cout << &context << std::endl;

    // Lexer::LoadInputString("if something 135 then something else something");
    // Lexer::LoadInputString("a > test <> 100 = 1000");
    // Lexer::LoadInputString("( a > test ) <> ( 100 = 1000 )");
    // Lexer::LoadInputString("if a then if x then y else c");
    // Lexer::LoadInputString("if a then if x then y - 30 else c + 100");
    Lexer::LoadInputString("var test $ type; test := a - b + 1000;");

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
        if (DeclarationStatementNode* declStmt = dynamic_cast<DeclarationStatementNode*>(top)) {
            std::cout << "VISITED DECLARATION STATEMENT NODE: " << declStmt->mIdentifier << "(type: " << declStmt->mType << ")\n";
            stack.push_back(declStmt->mExpression);
        }
        if (StatementListNode* stmtList = dynamic_cast<StatementListNode*>(top)) {
            std::cout << "VISITED STATEMENT LIST OF SIZE " << stmtList->mStatements.size() << '\n';
            for (ASTNode* node : stmtList->mStatements) {
                stack.push_back(node);
            }
        }
    }

    return 0;
};