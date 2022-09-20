#include <iostream>
#include "llvm/IR/LLVMContext.h"

#include <vector>

#include "common.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

int main() {
    llvm::LLVMContext context;
    std::cout << &context << std::endl;

    // Lexer::LoadString("if something 135 then something else something");
    // Lexer::LoadString("a > test <> 100 = 1000");
    // Lexer::LoadString("( a > test ) <> ( 100 = 1000 )");
    // Lexer::LoadString("if a then if x then y else c");
    Lexer::LoadString("if a then if x then y - 30 else c + 100");

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
    }

    return 0;
};