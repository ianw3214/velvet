#include "lexer.h"

namespace {
    // Current string being analyzed
    std::string currString;
    int curr_index = 0;
}

void Lexer::LoadString(const std::string& string) {
    currString = string;
    curr_index = 0;
}

Lexeme Lexer::getLexeme() {
    int lookahead = curr_index;
    Token token = Token::INVALID;
    char lookahead_char = currString[lookahead];
    switch (lookahead_char) {
    case ' ':
    case '\t':
    case '\n':
    {
        while ((lookahead_char == ' ' || lookahead_char == '\t' || lookahead_char == '\n') && lookahead < currString.size()) {
            lookahead_char = currString[++lookahead];
        }
        token = Token::WHITESPACE;
    } break;
    case '<':
    case '>':
    case '=':
    {
        lookahead_char = currString[++lookahead];
        if (lookahead < currString.size()) {
            if (lookahead_char == '=' || lookahead_char == '>' || lookahead_char == '<') {
                lookahead++;
            }
        }
        token = Token::RELOP;
    } break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
        while ((lookahead_char >= '0' && lookahead_char <= '9') && lookahead < currString.size()) {
            lookahead_char = currString[++lookahead];
        }
        token = Token::NUM;
    } break;
    default:
    {
        while ((lookahead_char != ' ' && lookahead_char != '\t' && lookahead_char != '\n') && lookahead < currString.size()) {
            lookahead_char = currString[++lookahead];
        }
        token = Token::ID;
    } break;
    }
    Lexeme result;
    size_t size = static_cast<size_t>(lookahead - curr_index);
    result.symbol = currString.substr(curr_index, size);
    result.token = token;
    curr_index = lookahead;

    if (result.token == Token::WHITESPACE) {
        result = Lexer::getLexeme();
    }

    return result;
}