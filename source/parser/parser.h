#pragma once

#include "common.h"
#include "AST/AST.h"

#include <functional>

namespace Parser {
	ASTNode* Parse();

    ASTNode* ParseExpr();
    ASTNode* ParseExprPost(ASTNode * left);
    ASTNode* ParseTerm();
}