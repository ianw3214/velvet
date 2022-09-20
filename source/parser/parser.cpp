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
    return ParseExpr();
}

ASTNode* Parser::ParseExpr() {
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::IF) {
        return ParseIfExpr();
    }
    else {
        return ParseRelExpr();
    }
    return nullptr;
}

ASTNode* Parser::ParseIfExpr() {
    Lexeme lexeme = Lexer::getLexeme();
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
