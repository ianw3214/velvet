#pragma once

#include "common.h"
#include "AST/AST.h"

#include <functional>

namespace Parser {
	ASTNode* Parse();

    ASTNode* ParseStatementList();
    ASTNode* ParseStatement();
    ASTNode* ParseFunctionDeclaration();
    ASTNode* ParseVariableDeclaration();
    ASTNode* ParseAssignmentStmt();
    ASTNode* ParseExpr();
    ASTNode* ParseBlockExpr();
    ASTNode* ParseIfExpr();
    ASTNode* ParseLoopExpr();
    ASTNode* ParseRelExpr();
    ASTNode* ParseBinExpr();
    ASTNode* ParseBinExprRHS(ASTNode* left);
    ASTNode* ParseTerm();
}