#pragma once

#include "common.h"
#include "AST/AST.h"

#include <functional>

namespace Parser {
	ASTNode* Parse();

    ASTNode* ParseStatementList();
    ASTNode* ParseStatement();
    ASTNode* ParseDeclarationStmt();
    ASTNode* ParseAssignmentStmt();
    ASTNode* ParseExpr();
    ASTNode* ParseIfExpr();
    ASTNode* ParseRelExpr();
    ASTNode* ParseBinExpr();
    ASTNode* ParseBinExprRHS(ASTNode* left);
    ASTNode* ParseTerm();
}