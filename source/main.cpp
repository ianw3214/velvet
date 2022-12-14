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
    // Lexer::LoadInputString("fn testfunc () -> type { var test $ type; test := a - b + 1000; if 10 + 10 then b else c };");
    // Lexer::LoadInputString("fn testfunc () -> type { if 10 + 10 then 20 else 30 };");
    // Lexer::LoadInputString("fn main() -> type { loop { if 100 then 15 else 20 } }");
    // Lexer::LoadInputString("fn main(argc $ int, argv $ str) -> type { var test $ type := 15; assign argc := 15; loop { if argc then test else argc } }");
    // Lexer::LoadInputString("fn sum(a $ i32, b $ i32) -> i32 { a + b }");
    // Lexer::LoadInputString("fn testfunc() -> i32 { var testvar $ i32 := 10; testvar + 10 }");
    // Lexer::LoadInputString("fn sum(a $ i32, b $ i32) -> i32 { a + b }");
    Lexer::LoadInputString("fn toOne(num $ i32) -> i32 { if num then 1 else 0 }");
    
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
        if (LoopExpressionNode* loopExpr = dynamic_cast<LoopExpressionNode*>(top)) {
            std::cout << "VISITED LOOP EXPR NODE\n";
            stack.push_back(loopExpr->mBlockNode);
        }
        if (RelationalExpressionNode* relOp = dynamic_cast<RelationalExpressionNode*>(top)) {
            std::cout << "VISITED REL OPERATOR NODE: " << "relOp->mOperator" << '\n';
            stack.push_back(relOp->mRight);
            stack.push_back(relOp->mLeft);
        }
        if (BinaryExpressionNode* binOp = dynamic_cast<BinaryExpressionNode*>(top)) {
            std::cout << "VISITED REL OPERATOR NODE: " << "binOp->mOperator" << '\n';
            stack.push_back(binOp->mRight);
            stack.push_back(binOp->mLeft);
        }
        if (AssignmentExpressionNode* assignExpr = dynamic_cast<AssignmentExpressionNode*>(top)) {
            std::cout << "VISITED ASSIGNMENT STATEMENT NODE: " << assignExpr->mIdentifier << '\n';
            stack.push_back(assignExpr->mExpression);
        }
        if (VariableDeclarationNode* declStmt = dynamic_cast<VariableDeclarationNode*>(top)) {
            std::cout << "VISITED VARIABLE DECLARATION NODE: " << declStmt->mIdentifier << " (type: " << declStmt->mType << ")\n";
            stack.push_back(declStmt->mExpression);
        }
        if (ExpressionListNode* exprList = dynamic_cast<ExpressionListNode*>(top)) {
            std::cout << "VISITED EXPRESSION LIST OF SIZE " << exprList->mExpressions.size() << '\n';
            for (ASTNode* node : exprList->mExpressions) {
                stack.push_back(node);
            }
        }
        if (FunctionDeclNode* funcDecl = dynamic_cast<FunctionDeclNode*>(top)) {
            std::cout << "VISITED FUNCTION NODE W/ NAME: " << funcDecl->mName << '\n';
            stack.push_back(funcDecl->mBlockExpr);
            stack.push_back(funcDecl->mType);
            stack.push_back(funcDecl->mParamList);
        }
        if (BlockExpressionNode* blockExpr = dynamic_cast<BlockExpressionNode*>(top)) {
            std::cout << "VISITED BLOCK EXPRESSION NODE\n";
            stack.push_back(blockExpr->mExpressionList);
        }
        if (FunctionParamListNode* funcParamList = dynamic_cast<FunctionParamListNode*>(top)) {
            std::cout << "VISITED FUNCTION PARAMETER LIST OF SIZE " << funcParamList->mParams.size() << '\n';
            for (ASTNode* node : funcParamList->mParams) {
                stack.push_back(node);
            }
        }
        if (FunctionParamNode* funcParam = dynamic_cast<FunctionParamNode*>(top)) {
            std::cout << "VISITED FUNCTION PARAMETER " << funcParam->mName << '\n';
            stack.push_back(funcParam->mType);
        }
        if (TypeNode* type = dynamic_cast<TypeNode*>(top)) {
            if (type->mTypeClass == Token::TYPE_I32) std::cout << "VISITED TYPE NODE (i32)\n";
            if (type->mTypeClass == Token::TYPE_F32) std::cout << "VISITED TYPE NODE (f32)\n";
            if (type->mTypeClass == Token::TYPE_BOOL) std::cout << "VISITED TYPE NODE (bool)\n";
            if (type->mTypeClass == Token::ID) std::cout << "VISITED TYPE NODE (" + type->mIdentifier + ")\n";
        }
    }

    initLLVM();
    base->Codegen();
    printLLVM();

    return 0;
};