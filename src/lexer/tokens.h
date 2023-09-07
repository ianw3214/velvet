#pragma once

enum class Token {
    ID,
    NUM,

    COMMA,
    COLON,
    SEMICOLON,
    LEFT_PARENTHESIS,
    RIGHT_PARENTHESIS,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    LEFT_SQUARE_BRACKET,
    RIGHT_SQUARE_BRACKET,

    FUNC_DEF,
    FUNC_RETURN,
    VAR_DEF,
    ASSIGN,
    ARRAY_DECAY,

    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    EQUALS,
    NOT_EQUALS,
    GREATER,
    GREATER_EQUALS,
    LESS,
    LESS_EQUALS,
    AND,
    OR,
    // TODO: Boolean operators (and/or)

    IF,
    THEN,
    ELSE,
    LOOP,
    BREAK,

    TYPE_I32,
    TYPE_F32,
    TYPE_BOOL,

    TOK_EOF
};