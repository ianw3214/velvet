#pragma once

#include "error/errorHandler.h"

#include "lexer/lexer.h"
#include "parser/ast.h"

class Parser {
    Lexer mLexer;
    ErrorHandler& mErrorHandler;
public:
    Parser(std::string input, ErrorHandler& handler);

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