#include "parser.h"

#include <iostream>

#include "lexer/lexer.h"

void Parser::Parse() {
    ParseExpr();
}

void Parser::ParseExpr() {
    ParseTerm();
    ParseExprPost();
}

void Parser::ParseExprPost(){
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::RELOP) {
        Lexer::getLexeme();
        std::cout << "RELOP: " << lexeme.symbol << '\n';
        ParseExpr();
        ParseExprPost();
    }
}

void Parser::ParseTerm(){
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token == Token::ID) {
        if (lexeme.symbol == "(") {
            ParseExpr();
            lexeme = Lexer::getLexeme();
            if (lexeme.symbol != ")") {
                std::cout << "ERROR! Expected ')'\n";
            }
        }
        else {
            std::cout << "ID: " << lexeme.symbol << '\n';
        }
    }
    if (lexeme.token == Token::NUM) {
        std::cout << "NUM: " << lexeme.symbol << '\n';
    }
}
