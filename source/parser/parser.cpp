#include "parser.h"

#pragma optimize("", off)

#include <iostream>

void Parser::Parse(std::function<Lexeme()> getTokenCallback) {
    ParseExpr(getTokenCallback);
}

void Parser::ParseExpr(std::function<Lexeme()> getTokenCallback) {
    ParseTerm(getTokenCallback);
    ParseExprPost(getTokenCallback);
}

void Parser::ParseExprPost(std::function<Lexeme()> getTokenCallback){
    Lexeme lexeme = getTokenCallback();
    if (lexeme.token == Token::RELOP) {
        std::cout << "RELOP: " << lexeme.symbol << '\n';
        ParseExpr(getTokenCallback);
        ParseExprPost(getTokenCallback);
    }
}

void Parser::ParseTerm(std::function<Lexeme()> getTokenCallback){
    Lexeme lexeme = getTokenCallback();
    if (lexeme.token == Token::ID) {
        if (lexeme.symbol == "(") {
            ParseExpr(getTokenCallback);
            lexeme = getTokenCallback();
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
