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
        constexpr int end_index = static_cast<int>(Token::MINUS);
        const int target = static_cast<int>(token);
        return target >= start_index && target <= end_index;
    }
}

ASTNode* Parser::Parse() {
    return ParseExprList();
    // return ParseExpr();
}

ASTNode* Parser::ParseExprList() {
    ASTNode* expression = ParseExpr();
    if (!expression) {
        std::cout << "Error! Expected expression to be parsed\n";
        return nullptr;
    }
    ASTNode* expression_list_post = ParseExprListPost();
    // TODO: There's probably a more efficient way to do this
    std::vector<ASTNode*> expressions = { expression };
    if (ExpressionListNode* list = dynamic_cast<ExpressionListNode*>(expression_list_post)) {
        expressions.insert(expressions.end(), list->mExpressions.begin(), list->mExpressions.end());
    }
    return new ExpressionListNode(std::move(expressions));
}

ASTNode* Parser::ParseExprListPost() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::EXPRESSION_END) {
        return nullptr;
    }
    ASTNode* expression = ParseExpr();
    if (!expression) {
        std::cout << "Error! Expected expression to be parsed\n";
        return nullptr;
    }
    ASTNode* expression_list_post = ParseExprListPost();
    // TODO: There's probably a more efficient way to do this
    std::vector<ASTNode*> expressions = { expression };
    if (ExpressionListNode* list = dynamic_cast<ExpressionListNode*>(expression_list_post)) {
        expressions.insert(expressions.end(), list->mExpressions.begin(), list->mExpressions.end());
    }
    return new ExpressionListNode(std::move(expressions));
}

ASTNode* Parser::ParseFunctionDeclaration() {
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
    ASTNode* funcParams = ParseFunctionParamList();
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::RIGHT_BRACKET) {
        std::cout << "Error! Expected right bracket\n";
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::FN_TYPE_RESULT) {
        std::cout << "Error! Expected -> for function type result\n";
    }
    ASTNode* type = ParseType();
    ASTNode* block = ParseBlockExpr();
    return new FunctionDeclNode(identifier, type, funcParams, block);
}

ASTNode* Parser::ParseFunctionParamList() {
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::ID) {
        ASTNode* param = ParseFunctionParam();
        ASTNode* paramPost = ParseFunctionParamListPost();
        std::vector<ASTNode*> params = { param };
        // TODO: There's probably a more efficient way to do this
        if (FunctionParamListNode* node = dynamic_cast<FunctionParamListNode*>(paramPost)) {
            params.insert(params.end(), node->mParams.begin(), node->mParams.end());
        }
        return new FunctionParamListNode(std::move(params));
    }
    return nullptr;
}

ASTNode* Parser::ParseFunctionParamListPost() {
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::COMMA) {
        Lexer::getLexeme();
        ASTNode* param = ParseFunctionParam();
        ASTNode* paramPost = ParseFunctionParamListPost();
        std::vector<ASTNode*> params = { param };
        // TODO: There's probably a more efficient way to do this
        if (FunctionParamListNode* node = dynamic_cast<FunctionParamListNode*>(paramPost)) {
            params.insert(params.end(), node->mParams.begin(), node->mParams.end());
        }
        return new FunctionParamListNode(std::move(params));
    }
    return nullptr;
}
ASTNode* Parser::ParseFunctionParam() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "Error! Expected id token\n";
    }
    std::string identifier = lexeme.symbol;
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::TYPE_DECL) {
        std::cout << "Error! Expected type declaration ($)\n";
    }
    ASTNode* type = ParseType();
    return new FunctionParamNode(identifier, type);
}

ASTNode* Parser::ParseVariableDeclaration() {
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
    ASTNode* type = ParseType();
    // optional assignment after var declaration
    ASTNode* opt_assign = nullptr;
    lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::ASSIGNMENT) {
        Lexer::getLexeme();
        opt_assign = ParseExpr();
    }
    return new VariableDeclarationNode(identifier, type, opt_assign);
}

ASTNode* Parser::ParseAssignmentExpr() {
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
    case Token::LEFT_CURLY_BRACKET: {
        return ParseBlockExpr();
    } break;
    case Token::IF: {
        return ParseIfExpr();
    } break;
    case Token::LOOP: {
        return ParseLoopExpr();
    } break;
    default: {
        return ParseRelExpr();
    }
    }
    return nullptr;
}

ASTNode* Parser::ParseBlockExpr() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::LEFT_CURLY_BRACKET) {
        std::cout << "ERROR! Expected left curly bracket\n";
    }
    ASTNode* expressions = ParseExprList();
    return new BlockExpressionNode(expressions);
}

ASTNode* Parser::ParseIfExpr() {
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

ASTNode* Parser::ParseLoopExpr() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::LOOP) {
        std::cout << "ERROR! Expected 'loop'\n";
    }
    ASTNode* block = ParseBlockExpr();
    return new LoopExpressionNode(block);
}

ASTNode* Parser::ParseRelExpr() {
    ASTNode* lhs = ParseBinExpr();
    Lexeme lexeme = Lexer::peekLexeme();
    if (_isRelationalOperator(lexeme.token)) {
        Lexer::getLexeme();
        ASTNode* rhs = ParseBinExpr();
        return new RelationalOperatorNode(lhs, rhs, lexeme.token);
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
        ASTNode* binop = new BinaryOperatorNode(left, right, lexeme.token);
        ASTNode* opt_post = ParseBinExprRHS(binop);
        return opt_post ? opt_post : binop;
    }
    return nullptr;
}

ASTNode* Parser::ParseType() {
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
