#include <iostream>

#include <vector>
#include <fstream>
#include <sstream>

#include "common.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

int main(int argc, char* argv[]) {
    std::string inputString = "fn sum(a $ i32, b $ i32) -> i32 { a * b }";

    // Decide which file to load based on command line input
    if (argc > 1) {
        std::string filename(argv[1]);

        std::ifstream inputFile(filename);
        if (inputFile.is_open()) {
            std::stringstream buffer;
            buffer << inputFile.rdbuf();
            inputString = buffer.str();
        }
    }

    Lexer::LoadInputString(inputString);

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
        if (BinaryOperatorNode* binOp = dynamic_cast<BinaryOperatorNode*>(top)) {
            std::cout << "VISITED BINARY OPERATOR NODE: " << "relOp->mOperator (SHOULD IMPLEMENT THIS)" << '\n';
            stack.push_back(binOp->mRight);
            stack.push_back(binOp->mLeft);
        }
        if (VariableDeclarationNode* declStmt = dynamic_cast<VariableDeclarationNode*>(top)) {
            std::cout << "VISITED VARIABLE DECLARATION NODE: " << declStmt->mIdentifier << '\n';
            stack.push_back(declStmt->mType);
            if (declStmt->mExpression) {
                stack.push_back(declStmt->mExpression);
            }
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
            std::cout << "  TYPE IS ARRAY: " << (type->mIsArray ? "TRUE" : "FALSE") << (type->mIsArray && type->mArraySize ? '(' + type->mArraySize->mNumber + ')' : "") << '\n';
        }
        if (FunctionCallNode* funcCall = dynamic_cast<FunctionCallNode*>(top)) {
            std::cout << "VISITED FUNCTION CALL " << funcCall->mFuncName << '\n';
            if (funcCall->mArgumentList) {
                for (ASTNode* node : funcCall->mArgumentList->mArguments) {
                    stack.push_back(node);
                }
            }
        }
        if (ArrayAccessNode* arrayAccess = dynamic_cast<ArrayAccessNode*>(top)) {
            std::cout << "VISITED ARRAY ACCESS NODE: " << arrayAccess->mName << '\n';
            stack.push_back(arrayAccess->mArrayIndexExpr);
        }
    }

    initLLVM();
    base->Codegen();
    printLLVM();

    return 0;
};