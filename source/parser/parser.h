#pragma once

#include "common.h"
#include "AST/AST.h"

#include <functional>

namespace Parser {
	ASTNode* Parse();

    ASTNode* ParseExprList();
    ASTNode* ParseExprListPost();
    ASTNode* ParseFunctionDeclaration();
    ASTNode* ParseFunctionParamList();
    ASTNode* ParseFunctionParamListPost();
    ASTNode* ParseFunctionParam();
    ASTNode* ParseVariableDeclaration();
    ASTNode* ParseAssignmentExpr();
    ASTNode* ParseExpr();
    ASTNode* ParseBlockExpr();
    ASTNode* ParseIfExpr();
    ASTNode* ParseLoopExpr();
    ASTNode* ParseRelExpr();
    ASTNode* ParseBinExpr();
    ASTNode* ParseBinExprRHS(ASTNode* left);
    ASTNode* ParseTerm();
}