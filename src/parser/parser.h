#pragma once

#include <vector>

#include "error/errorHandler.h"

#include "lexer/lexer.h"
#include "parser/ast.h"

class Parser {
    Lexer mLexer;
    // TODO: This should probably contain more than just function definitions
    std::vector<FunctionDefinitionNode> mTopLevelFunctions;

    ErrorHandler& mErrorHandler;
public:
    Parser(std::string input, ErrorHandler& handler);

    std::vector<FunctionDefinitionNode>& parseAll();

    ExpressionNodeOwner parseExpression();
    IdentifierNode parseIdentifier();

    ExpressionNodeOwner parsePrimary();
    VariableAccessNode parseVariableAccess();
    NumberNode parseNumber();
    ScopeNode parseScope();
    ConditionalNode parseConditional();

    BinaryOperationNode parseBinaryOperation(ExpressionNodeOwner left);
    VariableDefinitionNode parseVariableDefinition();
    AssignmentNode parseAssignment(VariableAccessNode&& variable);

    FunctionDefinitionNode parseFunctionDefinition();

private:
    bool _checkAndConsumeToken(Token target);
};