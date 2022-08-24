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

    // operators
    RELOP,

    TOKEN_EOF
};

struct Lexeme {
    Token token;
    std::string symbol;
};