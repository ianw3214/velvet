#pragma once

#include "common.h"
#include "AST/AST.h"

#include <functional>

namespace Parser {
	ASTNode* Parse();

    ExpressionListNode* ParseExprList();
    ExpressionListNode* ParseExprListPost();
    FunctionDeclNode* ParseFunctionDeclaration();
    FunctionParamListNode* ParseFunctionParamList();
    FunctionParamListNode* ParseFunctionParamListPost();
    FunctionParamNode* ParseFunctionParam();
    FunctionCallNode* ParseFunctionCall();
    FunctionArgumentListNode* ParseFunctionArgList();
    FunctionArgumentListNode* ParseFunctionArgListPost();
    VariableDeclarationNode* ParseVariableDeclaration();
    ASTNode* ParseExpr();
    BlockExpressionNode* ParseBlockExpr();
    IfExpressionNode* ParseIfExpr();
    LoopExpressionNode* ParseLoopExpr();
    ReturnExpressionNode* ParseReturnExpr();
    ASTNode* ParseBinopExpr();
    ASTNode* ParseBinopExprPost(ASTNode* left);
    TypeNode* ParseType();
    ASTNode* ParseTerm();
    ASTNode* ParseArrayAccess();
}