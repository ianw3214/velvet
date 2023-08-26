#include "parser.h"

#include <unordered_map>

namespace {
    const std::unordered_map<Token, int> binaryOperators = {
        { Token::EQUALS, 0 },
        { Token::NOT_EQUALS, 0 },
        { Token::GREATER, 0 },
        { Token::GREATER_EQUALS, 0 },
        { Token::LESS, 0 },
        { Token::LESS_EQUALS, 0 },
        { Token::PLUS, 5 },
        { Token::MINUS, 5 },
        { Token::MULTIPLY, 10 },
        { Token::DIVIDE, 10 }
    };
}

Parser::Parser(std::string input, ErrorHandler& handler) : mLexer(input), mErrorHandler(handler) {}

std::vector<FunctionDefinitionNode>& Parser::parseAll() {
    while(mLexer.getCurrToken() == Token::FUNC_DEF) {
        mTopLevelFunctions.emplace_back(std::move(parseFunctionDefinition()));
    }
    return mTopLevelFunctions;
}

/// ExpressionNode
///     ::= Primary
///     ::= BinaryOperation
ExpressionNodeOwner Parser::parseExpression() {
    ExpressionNodeOwner primary = parsePrimary();
    
    // check if primary should be used for binary operation
    Token currToken = mLexer.getCurrToken();
    if (binaryOperators.find(currToken) != binaryOperators.end()) {
        return std::make_unique<BinaryOperationNode>(parseBinaryOperation(std::move(primary)));
    }
    return primary;
}

/// IdentifierNode
///   ::= identifier
IdentifierNode Parser::parseIdentifier() {
    if (mLexer.getCurrToken() == Token::ID) {
        IdentifierNode identifier = IdentifierNode{ mLexer.getCurrTokenStr() };
        mLexer.consumeToken();
        return identifier;
    }
    else {
        mErrorHandler.logError("Expected 'id' token when parsing identifier");
        return IdentifierNode{};
    }
}

/// Primary
///   ::= VariableAccessNode
///   ::= NumberNode
///   ::= ScopeNode
///   ::= ConditionalNode
///   ::= VariableDefinitionNode
///   ::= LoopNode
///   ::= BreakNode
ExpressionNodeOwner Parser::parsePrimary() {
    switch(mLexer.getCurrToken()) {
        case Token::ID: {
            VariableAccessNode variable = parseVariableAccess();
            if (mLexer.getCurrToken() == Token::ASSIGN) {
                return std::make_unique<AssignmentNode>(parseAssignment(std::move(variable)));
            }
            else {
                return std::make_unique<VariableAccessNode>(std::move(variable));
            }
        } break;
        case Token::NUM: {
            return std::make_unique<NumberNode>(parseNumber());
        } break;
        case Token::LEFT_BRACKET: {
            return std::make_unique<ScopeNode>(parseScope());
        } break;
        case Token::LEFT_SQUARE_BRACKET: {
            return std::make_unique<ArrayValueNode>(parseArrayValue());
        } break;
        case Token::IF: {
            return std::make_unique<ConditionalNode>(parseConditional());
        } break;
        case Token::VAR_DEF: {
            return std::make_unique<VariableDefinitionNode>(parseVariableDefinition());
        } break;
        case Token::LOOP: {
            return std::make_unique<LoopNode>(parseLoop());
        } break;
        case Token::BREAK: {
            return std::make_unique<BreakNode>(parseBreak());
        } break;
        default: {
            mErrorHandler.logError("Unexpected token when parsing primary.");
            return std::unique_ptr<VariableAccessNode>();
        } break;
    }
}

/// VariableAccessNode ::= IdentifierNode ('[' ExpressionNode ']')?
VariableAccessNode Parser::parseVariableAccess() {
    IdentifierNode identifier = parseIdentifier();
    // TODO: Need to figure out how to handle chained brackets (e.g. arrayVar[2][3][4])
    if (_checkAndConsumeToken(Token::LEFT_SQUARE_BRACKET)) {
        ExpressionNodeOwner expression = parseExpression();
        if (!_checkAndConsumeToken(Token::RIGHT_SQUARE_BRACKET)) {
            mErrorHandler.logError("Expected ']' at the end of array access");
            return VariableAccessNode{ identifier, std::move(expression), std::nullopt };
        }
        return VariableAccessNode{ identifier, std::move(expression), std::nullopt };
    }
    if (_checkAndConsumeToken(Token::LEFT_PARENTHESIS)) {
        std::vector<ExpressionNodeOwner> argExpressions;
        while(!_checkAndConsumeToken(Token::RIGHT_PARENTHESIS)) {
            argExpressions.emplace_back(parseExpression());
            if (!_checkAndConsumeToken(Token::COMMA) && mLexer.getCurrToken() != Token::RIGHT_PARENTHESIS) {
                mErrorHandler.logError("Expected ',' between argument expressions for function call parameter");
                return VariableAccessNode{ identifier, std::nullopt, std::nullopt };
            }
        }
        return VariableAccessNode{ identifier, std::nullopt, std::move(argExpressions) };
    }
    return VariableAccessNode{ identifier, std::nullopt, std::nullopt };
}

/// AssignmentNode ::= MemoryAccessNode = ExpressionNode
AssignmentNode Parser::parseAssignment(VariableAccessNode&& variable) {
    MemoryLocationNode memoryLocation = MemoryLocationNode{ std::move(variable) };
    if (!_checkAndConsumeToken(Token::ASSIGN)) {
        mErrorHandler.logError("Expected assignment token '='");
        return AssignmentNode{ std::move(memoryLocation) };
    }
    ExpressionNodeOwner expression = parseExpression();
    return AssignmentNode{ std::move(memoryLocation), std::move(expression) };
}

/// LoopNode ::= 'loop' ScopeNode
LoopNode Parser::parseLoop() {
    if(!_checkAndConsumeToken(Token::LOOP)) {
        mErrorHandler.logError("Expected 'loop' token at start of loop");
        return LoopNode{};
    }
    if(!_checkAndConsumeToken(Token::LEFT_BRACKET)) {
        mErrorHandler.logError("Expected left bracket at the start of loop expression");
        return LoopNode{};
    }
    if (_checkAndConsumeToken(Token::RIGHT_BRACKET)) {
        mErrorHandler.logError("Empty loop is not valid code");
        return LoopNode{};
    }
    std::vector<ExpressionNodeOwner> expressions;
    while(mLexer.getCurrToken() != Token::RIGHT_BRACKET) {
        expressions.emplace_back(parseExpression());
        if (!_checkAndConsumeToken(Token::SEMICOLON)) {
            mErrorHandler.logError("Expressions in loop must end with a semicolon");
            return LoopNode{};
        }
    }
    if (!_checkAndConsumeToken(Token::RIGHT_BRACKET)) {
        mErrorHandler.logError("Expected right bracket at the end of loop expression");
    }
    return LoopNode { std::move(expressions) };
}

/// BreakNode ::= 'break'
BreakNode Parser::parseBreak() {
    if (!_checkAndConsumeToken(Token::BREAK)) {
        mErrorHandler.logError("Expected break token for break statement");
        return BreakNode{};
    }
    return BreakNode{};
}

/// NumberNode ::= number
NumberNode Parser::parseNumber() {
    if (mLexer.getCurrToken() == Token::NUM) {
        const std::string& numString = mLexer.getCurrTokenStr();
        if (numString.find('.') == std::string::npos) {
            const int numVal = std::stoi(mLexer.getCurrTokenStr());
            NumberNode number = NumberNode{ numVal };
            mLexer.consumeToken();
            return number;
        }
        else {
            const float numVal = std::stof(mLexer.getCurrTokenStr());
            NumberNode number = NumberNode{ numVal };
            mLexer.consumeToken();
            return number;
        }
    }
    else {
        mErrorHandler.logError("Expected numerical token when parsing number");
        return NumberNode{ 0 };
    }
}

/// ScopeNode ::= '{' (ExpressionNode ';')* '}'
ScopeNode Parser::parseScope() {
    if(!_checkAndConsumeToken(Token::LEFT_BRACKET)) {
        mErrorHandler.logError("Expected left bracket at the start of scope expression");
        return ScopeNode{};
    }
    if (_checkAndConsumeToken(Token::RIGHT_BRACKET)) {
        // empty scope
        return ScopeNode{};
    }
    std::vector<ExpressionNodeOwner> expressions;
    expressions.emplace_back(parseExpression());
    while(_checkAndConsumeToken(Token::SEMICOLON)) {
        expressions.emplace_back(parseExpression());
    }
    if(!_checkAndConsumeToken(Token::RIGHT_BRACKET)) {
        mErrorHandler.logError("Expected right bracket at the end of scope expression");
        return ScopeNode{};
    }
    return ScopeNode { std::move(expressions) };
}

/// ArrayValueNode ::= '[' (ExpressionNode, ',')* ']'
ArrayValueNode Parser::parseArrayValue() {
    if (!_checkAndConsumeToken(Token::LEFT_SQUARE_BRACKET)) {
        mErrorHandler.logError("Expected left square bracket at start of array value expression");
        return ArrayValueNode{};
    }
    std::vector<ExpressionNodeOwner> expressions;
    if (_checkAndConsumeToken(Token::RIGHT_SQUARE_BRACKET)) {
        // Handle empty array value
        return ArrayValueNode{};
    }
    expressions.emplace_back(parseExpression());
    while(_checkAndConsumeToken(Token::COMMA)) {
        expressions.emplace_back(std::move(parseExpression()));
    }
    if (!_checkAndConsumeToken(Token::RIGHT_SQUARE_BRACKET)) {
        mErrorHandler.logError("Expected right square bracket at end of array value expression");
        return ArrayValueNode{};
    }
    return ArrayValueNode{ std::move(expressions) };
}

/// ConditionalNode ::= 'if' ExpressionNode 'then' ExpressionNode ('else' ExpressionNode)?
ConditionalNode Parser::parseConditional() {
    if (!_checkAndConsumeToken(Token::IF)) {
        mErrorHandler.logError("Expected 'if' at the start of if expression");
        return ConditionalNode{};
    }
    ExpressionNodeOwner conditionExpression = parseExpression();
    if (!_checkAndConsumeToken(Token::THEN)) {
        mErrorHandler.logError("Expected 'then' after if condition");
        return ConditionalNode{};
    }
    ExpressionNodeOwner thenExpression = parseExpression();
    if (!_checkAndConsumeToken(Token::ELSE)) {
        return ConditionalNode{ std::move(conditionExpression), std::move(thenExpression), std::nullopt };
    }
    ExpressionNodeOwner elseExpression = parseExpression();
    return ConditionalNode{ std::move(conditionExpression), std::move(thenExpression), std::move(elseExpression) };
}

/// BinaryOperationNode ::= ExpressionNode op ExpressionNode
BinaryOperationNode Parser::parseBinaryOperation(ExpressionNodeOwner left) {
    // if we are in this function, assume that the curr token IS a binary operation
    Token binaryOperator = mLexer.getCurrToken();
    const int currPrecedence = binaryOperators.at(binaryOperator);
    mLexer.consumeToken();

    ExpressionNodeOwner right = parseExpression();
    // handling operator precedence if rhs is binary node as well
    if (auto binop = std::get_if<std::unique_ptr<BinaryOperationNode>>(&right)) {
        std::unique_ptr<BinaryOperationNode>& binaryOperation = *binop;
        const int nextPrecedence = binaryOperators.at(binaryOperation->mOperation);
        if (currPrecedence > nextPrecedence) {
            BinaryOperationNode newLHS{ std::move(left), std::move(binaryOperation->mLeft), binaryOperator };
            binaryOperation->mLeft = std::make_unique<BinaryOperationNode>(std::move(newLHS));
            // feels mega unsafe... maybe improve this somehow
            return std::move(*binaryOperation.release());
        }
    }
    return BinaryOperationNode{ std::move(left), std::move(right), binaryOperator };
}

/// VariableDefinitionNode ::= 'var' IdentifierNode (':=' ExpressionNode)?
VariableDefinitionNode Parser::parseVariableDefinition() {
    if (!_checkAndConsumeToken(Token::VAR_DEF)) {
        mErrorHandler.logError("Expected 'var' at the start of variable definition");
        return VariableDefinitionNode{};
    }
    IdentifierNode identifier = parseIdentifier();
    if (!_checkAndConsumeToken(Token::COLON)) {
        mErrorHandler.logError("Expected ':' to mark type of variable");
    }
    if (_checkAndConsumeToken(Token::LEFT_SQUARE_BRACKET)) {
        Token typeInfo = mLexer.getCurrToken();
        mLexer.consumeToken();
        if (!_checkAndConsumeToken(Token::SEMICOLON)) {
            mErrorHandler.logError("Expected ';' to separate array type and size");
            return VariableDefinitionNode{ identifier, typeInfo, -1, std::nullopt };
        }
        NumberNode number = parseNumber();
        int* arraySize = std::get_if<int>(&number.mNumber);
        if (!arraySize || *arraySize < 0) {
            mErrorHandler.logError("Expected non-negative integer value for array size");
            return VariableDefinitionNode{ identifier, typeInfo, -1, std::nullopt };
        }
        if (!_checkAndConsumeToken(Token::RIGHT_SQUARE_BRACKET)) {
            mErrorHandler.logError("Expected ']' to end array type definition");
            return VariableDefinitionNode{ identifier, typeInfo, *arraySize, std::nullopt };
        }
        if (!_checkAndConsumeToken(Token::ASSIGN)) {
            return VariableDefinitionNode{ identifier, typeInfo, *arraySize, std::nullopt };
        }
        // TODO: Maybe check that this is an arrayValue node, or even parse it directly?
        ExpressionNodeOwner initialArrayValue = parseExpression();
        return VariableDefinitionNode{ identifier, typeInfo, *arraySize, std::move(initialArrayValue) };
    }
    else {
        Token typeInfo = mLexer.getCurrToken();
        mLexer.consumeToken();
        if (!_checkAndConsumeToken(Token::ASSIGN)) {
            return VariableDefinitionNode{ identifier, typeInfo, -1, std::nullopt };
        }
        ExpressionNodeOwner initialValue = parseExpression();
        return VariableDefinitionNode{ identifier, typeInfo, -1, std::move(initialValue) };
    }
}

/// FunctionDefinitionNode ::= 'def' IdentifierNode '(' (IdentifierNode ',')* IdentifierNode? ')' expressionNode
FunctionDefinitionNode Parser::parseFunctionDefinition() {
    if (!_checkAndConsumeToken(Token::FUNC_DEF)) {
        mErrorHandler.logError("Expected 'def' at the start of function definition");
        return FunctionDefinitionNode{};
    }
    IdentifierNode identifier = parseIdentifier();
    if (!_checkAndConsumeToken(Token::LEFT_PARENTHESIS)) {
        mErrorHandler.logError("Expected left parenthesis in function definition");
        return FunctionDefinitionNode { identifier };
    }
    std::vector<std::pair<std::string, Token>> arguments;
    while (mLexer.getCurrToken() == Token::ID) {
        std::string name = mLexer.getCurrTokenStr();
        mLexer.consumeToken();
        if (!_checkAndConsumeToken(Token::COLON)) {
            mErrorHandler.logError("Expected ':' after argument name");
            return FunctionDefinitionNode{ identifier };
        }
        // TODO: Maybe want to validate token is type token here as well
        Token typeInfo = mLexer.getCurrToken();
        mLexer.consumeToken();
        arguments.emplace_back(name, typeInfo);
        if (mLexer.getCurrToken() == Token::COMMA) {
            mLexer.consumeToken();
            // Might want to validate that the next token is an identifier
            //  - or maybe it's fine to have trailing commas? probably not...
        }
    }
    if (!_checkAndConsumeToken(Token::RIGHT_PARENTHESIS)) {
        mErrorHandler.logError("Expected right parenthesis in function definition");
        return FunctionDefinitionNode { identifier };
    }
    // TODO: Maybe want to allow functions to _not_ specify a return type
    if (!_checkAndConsumeToken(Token::FUNC_RETURN)) {
        mErrorHandler.logError("Expected '@' for function return symbol");
        return FunctionDefinitionNode { identifier, arguments };
    }
    Token returnType = mLexer.getCurrToken();
    mLexer.consumeToken();
    ExpressionNodeOwner expression = parseExpression();
    return FunctionDefinitionNode{ identifier, arguments, returnType, std::move(expression) };
}

bool Parser::_checkAndConsumeToken(Token target) {
    if (mLexer.getCurrToken() != target) {
        // it's not an error if we don't find the token, simply return false
        return false;
    }
    mLexer.consumeToken();
    return true;
}