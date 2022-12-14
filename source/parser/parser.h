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
    VariableDeclarationNode* ParseVariableDeclaration();
    AssignmentExpressionNode* ParseAssignmentExpr();
    ASTNode* ParseExpr();
    BlockExpressionNode* ParseBlockExpr();
    IfExpressionNode* ParseIfExpr();
    LoopExpressionNode* ParseLoopExpr();
    ASTNode* ParseRelExpr();
    ASTNode* ParseBinExpr();
    ASTNode* ParseBinExprRHS(ASTNode* left);
    TypeNode* ParseType();
    ASTNode* ParseTerm();
}