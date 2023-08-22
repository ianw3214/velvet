#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <utility>

#include "lexer/tokens.h"

struct IdentifierNode {
    std::string mIdentifier;
};

struct VariableAccessNode;
struct NumberNode;
struct ScopeNode;
struct BinaryOperationNode;
struct ConditionalNode;
// these are really statements implemented as expressions with NO VALUE
struct VariableDefinitionNode;
struct AssignmentNode;

using ExpressionNodeOwner = std::variant<
    std::unique_ptr<VariableAccessNode>,
    std::unique_ptr<NumberNode>,
    std::unique_ptr<ScopeNode>,
    std::unique_ptr<ConditionalNode>,
    std::unique_ptr<BinaryOperationNode>,
    // statements that are implemented as definitions with NO VALUE
    std::unique_ptr<VariableDefinitionNode>,
    std::unique_ptr<AssignmentNode>
>;

struct VariableAccessNode {
    IdentifierNode mName;
    std::optional<ExpressionNodeOwner> mArrayIndex;
    std::optional<std::vector<ExpressionNodeOwner>> mCallArgs;
};

struct NumberNode {
    std::variant<int, float> mNumber;
};

struct ScopeNode {
    std::vector<ExpressionNodeOwner> mExpressionList;
};

struct ConditionalNode {
    ExpressionNodeOwner mCondition;
    ExpressionNodeOwner mThen;
    std::optional<ExpressionNodeOwner> mElse;
};

struct BinaryOperationNode {
    ExpressionNodeOwner mLeft;
    ExpressionNodeOwner mRight;
    Token mOperation;
};

struct VariableDefinitionNode {
    IdentifierNode mName;
    Token mType;
    int mArraySize; // let -1 array size represent not an array type
    std::optional<ExpressionNodeOwner> mInitialValue;
};

struct MemoryLocationNode {
    VariableAccessNode mVariable;
};

struct AssignmentNode {
    MemoryLocationNode mVariable;
    ExpressionNodeOwner mValue;
};

struct FunctionDefinitionNode {
    IdentifierNode mName;
    std::vector<std::pair<std::string, Token>> mArguments;
    Token mReturnType;
    ExpressionNodeOwner mExpression;
};