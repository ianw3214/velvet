#include "lexer.h"

#include <unordered_map>
#include <utility>

namespace {
    // Current string being analyzed
    std::string currString;
    int curr_index = 0;

    std::unordered_map<std::string, Token> keywords = {
        { "if", Token::IF },
        { "then", Token::THEN },
        { "else", Token::ELSE }
    };
}

std::pair<int, Token> _advanceLookahead(int lookahead) {
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
    return std::make_pair<>(lookahead, token);
}

void Lexer::LoadString(const std::string& string) {
    currString = string;
    curr_index = 0;
}

Lexeme Lexer::peekLexeme() {
    std::pair<int, Token> new_lookahead = _advanceLookahead(curr_index);

    Lexeme result;
    size_t size = static_cast<size_t>(new_lookahead.first - curr_index);
    result.symbol = currString.substr(curr_index, size);
    result.token = new_lookahead.second;

    if (result.token == Token::WHITESPACE) {
        int token_start = new_lookahead.first;
        new_lookahead = _advanceLookahead(new_lookahead.first);

        size_t size = static_cast<size_t>(new_lookahead.first - token_start);
        result.symbol = currString.substr(token_start, size);
        result.token = new_lookahead.second;
    }

    if (result.token == Token::ID) {
        auto entry = keywords.find(result.symbol);
        if (entry != keywords.end()) {
            result.token = entry->second;
        }
    }

    return result;
}

Lexeme Lexer::getLexeme() {
    std::pair<int, Token> new_lookahead = _advanceLookahead(curr_index);

    Lexeme result;
    size_t size = static_cast<size_t>(new_lookahead.first - curr_index);
    result.symbol = currString.substr(curr_index, size);
    result.token = new_lookahead.second;
    curr_index = new_lookahead.first;

    if (result.token == Token::WHITESPACE) {
        result = Lexer::getLexeme();
    }

    if (result.token == Token::ID) {
        auto entry = keywords.find(result.symbol);
        if (entry != keywords.end()) {
            result.token = entry->second;
        }
    }

    return result;
}