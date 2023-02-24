#pragma once

#include <string>

enum class Token {
    INVALID = -1,

    WHITESPACE,

    // declaration/assignment
    VAR_DECL,
    TYPE_DECL,
    FN_DECL,
    ASSIGNMENT,

    // keywords
    IF,
    THEN,
    ELSE,
    LOOP,

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
    COMMA,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    EXPRESSION_END,
    LEFT_CURLY_BRACKET,
    RIGHT_CURLY_BRACKET,
    LEFT_SQUARE_BRACKET,
    RIGHT_SQUARE_BRACKET,
    FN_TYPE_RESULT,

    // types
    TYPE_I32,
    TYPE_F32,
    TYPE_BOOL,
    TYPE_ID,

    TOKEN_EOF
};

struct Lexeme {
    Token token;
    std::string symbol;
};