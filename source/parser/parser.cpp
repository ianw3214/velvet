#include "parser.h"

#include <iostream>

#include "lexer/lexer.h"

namespace {
    bool _isRelationalOperator(Token token) {
        constexpr int start_index = static_cast<int>(Token::GREATER);
        constexpr int end_index = static_cast<int>(Token::NOT_EQUALS);
        const int target = static_cast<int>(token);
        return target >= start_index && target <= end_index;
    }

    bool _isBinaryOperator(Token token) {
        constexpr int start_index = static_cast<int>(Token::PLUS);
        constexpr int end_index = static_cast<int>(Token::DIVIDE);
        const int target = static_cast<int>(token);
        return target >= start_index && target <= end_index;
    }
}

ASTNode* Parser::Parse() {
    return ParseExprList();
}

ExpressionListNode* Parser::ParseExprList() {
    ASTNode* expression = ParseExpr();
    if (!expression) {
        std::cout << "Error! Expected expression to be parsed\n";
        return nullptr;
    }
    ExpressionListNode* expression_list_post = ParseExprListPost();
    // TODO: There's probably a more efficient way to do this
    std::vector<ASTNode*> expressions = { expression };
    if (expression_list_post) {
        expressions.insert(expressions.end(), expression_list_post->mExpressions.begin(), expression_list_post->mExpressions.end());
    }
    return new ExpressionListNode(std::move(expressions));
}

ExpressionListNode* Parser::ParseExprListPost() {
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token != Token::EXPRESSION_END) {
        return nullptr;
    }
    Lexer::getLexeme();
    ASTNode* expression = ParseExpr();
    if (!expression) {
        std::cout << "Error! Expected expression to be parsed\n";
        return nullptr;
    }
    ExpressionListNode* expression_list_post = ParseExprListPost();
    // TODO: There's probably a more efficient way to do this
    std::vector<ASTNode*> expressions = { expression };
    if (expression_list_post) {
        expressions.insert(expressions.end(), expression_list_post->mExpressions.begin(), expression_list_post->mExpressions.end());
    }
    return new ExpressionListNode(std::move(expressions));
}

FunctionDeclNode* Parser::ParseFunctionDeclaration() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::FN_DECL) {
        std::cout << "Error! Expected function declaration(var)\n";
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "Error! Expected id token\n";
    }
    std::string identifier = lexeme.symbol;
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::LEFT_BRACKET) {
        std::cout << "Error! Expected left bracket\n";
    }
    FunctionParamListNode* funcParams = ParseFunctionParamList();
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::RIGHT_BRACKET) {
        std::cout << "Error! Expected right bracket\n";
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::FN_TYPE_RESULT) {
        std::cout << "Error! Expected -> for function type result\n";
    }
    TypeNode* type = ParseType();
    BlockExpressionNode* block = ParseBlockExpr();
    return new FunctionDeclNode(identifier, type, funcParams, block);
}

FunctionParamListNode* Parser::ParseFunctionParamList() {
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::ID) {
        FunctionParamNode* param = ParseFunctionParam();
        FunctionParamListNode* paramPost = ParseFunctionParamListPost();
        std::vector<FunctionParamNode*> params = { param };
        // TODO: There's probably a more efficient way to do this
        if (paramPost) {
            params.insert(params.end(), paramPost->mParams.begin(), paramPost->mParams.end());
        }
        return new FunctionParamListNode(std::move(params));
    }
    return nullptr;
}

FunctionParamListNode* Parser::ParseFunctionParamListPost() {
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::COMMA) {
        Lexer::getLexeme();
        FunctionParamNode* param = ParseFunctionParam();
        FunctionParamListNode* paramPost = ParseFunctionParamListPost();
        std::vector<FunctionParamNode*> params = { param };
        // TODO: There's probably a more efficient way to do this
        if (paramPost) {
            params.insert(params.end(), paramPost->mParams.begin(), paramPost->mParams.end());
        }
        return new FunctionParamListNode(std::move(params));
    }
    return nullptr;
}
FunctionParamNode* Parser::ParseFunctionParam() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "Error! Expected id token\n";
    }
    std::string identifier = lexeme.symbol;
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::TYPE_DECL) {
        std::cout << "Error! Expected type declaration ($)\n";
    }
    TypeNode* type = ParseType();
    return new FunctionParamNode(identifier, type);
}

FunctionCallNode* Parser::ParseFunctionCall() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "Error! Expected id node\n";
    }
    std::string funcName = lexeme.symbol;
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::LEFT_BRACKET) {
        std::cout << "Error! Expected left bracket\n";
    }
    lexeme = Lexer::peekLexeme();
    // Handle empty arguments
    if (lexeme.token == Token::RIGHT_BRACKET) {
        return new FunctionCallNode(funcName, nullptr);
    }
    FunctionArgumentListNode* argList = ParseFunctionArgList();
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::RIGHT_BRACKET) {
        std::cout << "Error! Expected right bracket\n";
    }
    return new FunctionCallNode(funcName, argList);
}

FunctionArgumentListNode* Parser::ParseFunctionArgList() {
    // TODO: Need to check if we can actually parse an expression here
    ASTNode* expression = ParseExpr();
    FunctionArgumentListNode* post = ParseFunctionArgListPost();
    std::vector<ASTNode*> expressions = { expression };
    if (post) {
        expressions.insert(expressions.end(), post->mArguments.begin(), post->mArguments.end());
    }
    return new FunctionArgumentListNode(std::move(expressions));
}

FunctionArgumentListNode* Parser::ParseFunctionArgListPost() {
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::COMMA) {
        Lexer::getLexeme(); // consume the comma if read
        ASTNode* expression = ParseExpr();
        FunctionArgumentListNode* post = ParseFunctionArgListPost();
        std::vector<ASTNode*> expressions = { expression };
        if (post) {
            expressions.insert(expressions.end(), post->mArguments.begin(), post->mArguments.end());
        }
        return new FunctionArgumentListNode(std::move(expressions));
    }
    return nullptr;
}

VariableDeclarationNode* Parser::ParseVariableDeclaration() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::VAR_DECL) {
        std::cout << "Error! Expected variable declaration (var)\n";
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "Error! Expected id token\n";
    }
    std::string identifier = lexeme.symbol;
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::TYPE_DECL) {
        std::cout << "Error! Expected type declaration ($)\n";
    }
    TypeNode* type = ParseType();
    // optional assignment after var declaration
    ASTNode* opt_assign = nullptr;
    lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::ASSIGNMENT) {
        Lexer::getLexeme();
        opt_assign = ParseExpr();
    }
    return new VariableDeclarationNode(identifier, type, opt_assign);
}

AssignmentExpressionNode* Parser::ParseAssignmentExpr() {
    Lexer::getLexeme();
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "ERROR! Expected id token\n";
        return nullptr;
    }
    std::string identifier = lexeme.symbol;
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ASSIGNMENT) {
        std::cout << "ERROR! Expected assignment operator (:=) \n";
        return nullptr;
    }
    ASTNode* expr = ParseExpr();
    return new AssignmentExpressionNode(identifier, expr);
}

ASTNode* Parser::ParseExpr() {
    Lexeme lexeme = Lexer::peekLexeme();
    switch (lexeme.token) {
    case Token::FN_DECL: {
        return ParseFunctionDeclaration();
    } break;
    case Token::VAR_DECL: {
        return ParseVariableDeclaration();
    } break;
    case Token::ASSIGN_DECL: {
        return ParseAssignmentExpr();
    } break;
    case Token::CALL_DECL: {
        return ParseFunctionCall();
    } break;
    case Token::LEFT_CURLY_BRACKET: {
        return ParseBlockExpr();
    } break;
    case Token::IF: {
        return ParseIfExpr();
    } break;
    case Token::LOOP: {
        return ParseLoopExpr();
    } break;
    case Token::ID: {
        lexeme = Lexer::peekLexeme(2);
        if (lexeme.token == Token::LEFT_BRACKET) {
            return ParseFunctionCall();
        }
    } // break;
    default: {
        return ParseRelExpr();
    }
    }
    return nullptr;
}

BlockExpressionNode* Parser::ParseBlockExpr() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::LEFT_CURLY_BRACKET) {
        std::cout << "ERROR! Expected left curly bracket\n";
    }
    ExpressionListNode* expressions = ParseExprList();
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::RIGHT_CURLY_BRACKET) {
        std::cout << "ERROR! Expected right curly bracket\n";
    }
    return new BlockExpressionNode(expressions);
}

IfExpressionNode* Parser::ParseIfExpr() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::IF) {
        std::cout << "ERROR! Expected 'if'\n";
    }
    ASTNode* condition = ParseExpr();
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::THEN) {
        std::cout << "ERROR! Expected 'then'\n";
    }
    ASTNode* then = ParseExpr();
    ASTNode* opt_else = nullptr;
    lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::ELSE) {
        Lexer::getLexeme();
        opt_else = ParseExpr();
    }
    return new IfExpressionNode(condition, then, opt_else);
}

LoopExpressionNode* Parser::ParseLoopExpr() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::LOOP) {
        std::cout << "ERROR! Expected 'loop'\n";
    }
    BlockExpressionNode* block = ParseBlockExpr();
    return new LoopExpressionNode(block);
}

ASTNode* Parser::ParseRelExpr() {
    ASTNode* lhs = ParseBinExpr();
    Lexeme lexeme = Lexer::peekLexeme();
    if (_isRelationalOperator(lexeme.token)) {
        Lexer::getLexeme();
        ASTNode* rhs = ParseBinExpr();
        return new RelationalExpressionNode(lhs, rhs, lexeme.token);
    }
    return lhs;
}

ASTNode* Parser::ParseBinExpr() {
    ASTNode* term = ParseTerm();
    ASTNode* opt_rhs = ParseBinExprRHS(term);
    return opt_rhs ? opt_rhs : term;
}

ASTNode* Parser::ParseBinExprRHS(ASTNode* left){
    Lexeme lexeme = Lexer::peekLexeme();
    if (_isBinaryOperator(lexeme.token)) {
        Lexer::getLexeme();
        ASTNode* right = ParseExpr();
        ASTNode* binop = new BinaryExpressionNode(left, right, lexeme.token);
        ASTNode* opt_post = ParseBinExprRHS(binop);
        return opt_post ? opt_post : binop;
    }
    return nullptr;
}

TypeNode* Parser::ParseType() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token == Token::TYPE_I32) {
        return new TypeNode(Token::TYPE_I32);
    }
    if (lexeme.token == Token::TYPE_F32) {
        return new TypeNode(Token::TYPE_F32);
    }
    if (lexeme.token == Token::TYPE_BOOL) {
        return new TypeNode(Token::TYPE_BOOL);
    }
    if (lexeme.token == Token::ID) {
        return new TypeNode(Token::ID, lexeme.symbol);
    }
    std::cout << "Could not parse type\n";
    return nullptr;
}

ASTNode* Parser::ParseTerm(){
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token == Token::LEFT_BRACKET) {
        ASTNode* expr = ParseExpr();
        lexeme = Lexer::getLexeme();
        if (lexeme.token != Token::RIGHT_BRACKET) {
            std::cout << "ERROR! Expected ')'\n";
        }
        return expr;
    }
    if (lexeme.token == Token::ID) {
        return new IdentifierNode(lexeme.symbol);
    }
    if (lexeme.token == Token::NUM) {
        return new NumberNode(lexeme.symbol);
    }
    // I think this should error?
    return nullptr;
}
