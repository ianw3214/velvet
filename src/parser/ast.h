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
struct ArrayValueNode;
struct BinaryOperationNode;
struct ConditionalNode;
// these are really statements implemented as expressions with NO VALUE
struct VariableDefinitionNode;
struct AssignmentNode;
struct LoopNode;
struct BreakNode;

using ExpressionNodeOwner = std::variant<
    std::unique_ptr<VariableAccessNode>,
    std::unique_ptr<NumberNode>,
    std::unique_ptr<ScopeNode>,
    std::unique_ptr<ArrayValueNode>,
    std::unique_ptr<ConditionalNode>,
    std::unique_ptr<BinaryOperationNode>,
    // statements that are implemented as definitions with NO VALUE
    std::unique_ptr<VariableDefinitionNode>,
    std::unique_ptr<AssignmentNode>,
    std::unique_ptr<LoopNode>,
    std::unique_ptr<BreakNode>
>;

struct VariableAccessNode {
    IdentifierNode mName;
    std::optional<std::vector<ExpressionNodeOwner>> mArrayIndices;
    std::optional<std::vector<ExpressionNodeOwner>> mCallArgs;
    bool mArrayDecay;
};

struct NumberNode {
    std::variant<int, float> mNumber;
};

struct ScopeNode {
    std::vector<ExpressionNodeOwner> mExpressionList;
};

struct ArrayValueNode {
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
    // Should type be its own identifier?
    Token mType;
    std::vector<size_t> mArraySizes; // let empty sizes represent not array type
    std::optional<ExpressionNodeOwner> mInitialValue;
};

struct MemoryLocationNode {
    VariableAccessNode mVariable;
};

struct AssignmentNode {
    MemoryLocationNode mVariable;
    ExpressionNodeOwner mValue;
};

struct LoopNode {
    std::vector<ExpressionNodeOwner> mExpressionList;
};

// Maybe want to do loop labels and breaking to certain labels in the future?
struct BreakNode {};

struct FunctionDefinitionNode {
    struct ArgType {
        Token mRawType;
        int mArraySize;
        bool mIsArrayDecay;
    };

    IdentifierNode mName;
    std::vector<std::pair<std::string, ArgType>> mArguments;
    Token mReturnType;
    ExpressionNodeOwner mExpression;
};