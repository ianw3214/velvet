#pragma once

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