#include "parser.h"

#include <iostream>

#include "lexer/lexer.h"

ASTNode* Parser::Parse() {
    return ParseExpr();
}

ASTNode* Parser::ParseExpr() {
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::IF) {
        Lexer::getLexeme();
        // std::cout << "IF\n";
        ASTNode * condition = ParseExpr();
        lexeme = Lexer::getLexeme();
        if (lexeme.token != Token::THEN) {
            std::cout << "ERROR! Expected 'then'\n";
        }
        // std::cout << "THEN\n";
        ASTNode * then = ParseExpr();
        ASTNode* opt_else = nullptr;
        lexeme = Lexer::peekLexeme();
        if (lexeme.token == Token::ELSE) {
            // std::cout << "ELSE\n";
            Lexer::getLexeme();
            opt_else = ParseExpr();
        }
        return new IfExpressionNode(condition, then, opt_else);
    }
    else {
        ASTNode* term = ParseTerm();
        ASTNode* opt_post = ParseExprPost(term);
        return opt_post ? opt_post : term;
    }
    return nullptr;
}

ASTNode* Parser::ParseExprPost(ASTNode* left){
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::RELOP) {
        Lexer::getLexeme();
        // std::cout << "RELOP: " << lexeme.symbol << '\n';
        ASTNode * right = ParseExpr();
        ASTNode* binop = new BinaryOperatorNode(left, right, lexeme.token);
        ASTNode * opt_post = ParseExprPost(binop);
        return opt_post ? opt_post : binop;
    }
    return nullptr;
}

ASTNode* Parser::ParseTerm(){
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token == Token::ID) {
        if (lexeme.symbol == "(") {
            ASTNode * expr = ParseExpr();
            lexeme = Lexer::getLexeme();
            if (lexeme.symbol != ")") {
                std::cout << "ERROR! Expected ')'\n";
            }
            return expr;
        }
        else {
            // std::cout << "ID: " << lexeme.symbol << '\n';
            return new IdentifierNode(lexeme.symbol);
        }
    }
    if (lexeme.token == Token::NUM) {
        // std::cout << "NUM: " << lexeme.symbol << '\n';
        return new NumberNode(lexeme.symbol);
    }
    // I think this should error?
    return nullptr;
}
