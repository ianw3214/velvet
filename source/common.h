#pragma once

#include <string>

enum class Token {
    INVALID = -1,

    WHITESPACE,

    // keywords
    IF,
    THEN,
    ELSE,

    // identifiers/literals
    ID,
    NUM,

    // binary operators
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,

    // relational operators
    GREATER,
    GREATER_EQ,
    LESS,
    LESS_EQ,
    EQUALS,
    NOT_EQUALS,

    // misc tokens
    LEFT_BRACKET,
    RIGHT_BRACKET,

    TOKEN_EOF
};

struct Lexeme {
    Token token;
    std::string symbol;
};