#include "lexer.h"

#include <unordered_map>
#include <utility>
    
namespace {
    // Current string being analyzed
    std::string currString;
    int curr_index = 0;

    int backtrack_index = 0;

    std::unordered_map<std::string, Token> keywords = {
        { "if", Token::IF },
        { "then", Token::THEN },
        { "else", Token::ELSE },
        { "loop", Token::LOOP },
        { "var", Token::VAR_DECL },
        { "fn", Token::FN_DECL },
        // TEMPORARY, remove when lexer can peek ahead more than 1 token
        { "assign", Token::ASSIGN_DECL }
    };

    inline bool _isAlphaNumerical(char c) {
        return (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9');
    }
}

std::pair<int, Token> _advanceLookahead(int lookahead) {
    Token token = Token::INVALID;
    if (lookahead == currString.size()) {
        return std::make_pair<>(lookahead, Token::TOKEN_EOF);
    }
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
    case ';': {
        lookahead_char = currString[++lookahead];
        token = Token::EXPRESSION_END;
    } break;
    case ':': {
        lookahead_char = currString[++lookahead];
        if (lookahead < currString.size()) {
            if (lookahead_char == '=') {
                lookahead++;
                token = Token::ASSIGNMENT;
                break;
            }
        }
    } break;
    case '$': {
        lookahead_char = currString[++lookahead];
        token = Token::TYPE_DECL;
    } break;
    case '(': {
        lookahead_char = currString[++lookahead];
        token = Token::LEFT_BRACKET;
    } break;
    case ')': {
        lookahead_char = currString[++lookahead];
        token = Token::RIGHT_BRACKET;
    } break;
    case '{': {
        lookahead_char = currString[++lookahead];
        token = Token::LEFT_CURLY_BRACKET;
    } break;
    case '}': {
        lookahead_char = currString[++lookahead];
        token = Token::RIGHT_CURLY_BRACKET;
    } break;
    case '+': {
        lookahead_char = currString[++lookahead];
        token = Token::PLUS;
    } break;
    case '-': {
        lookahead_char = currString[++lookahead];
        if (lookahead < currString.size()) {
            if (lookahead_char == '>') {
                lookahead++;
                token = Token::FN_TYPE_RESULT;
                break;
            }
        }
        token = Token::MINUS;
    } break;
    case '*': {
        lookahead_char = currString[++lookahead];
        token = Token::MULTIPLY;
    } break;
    case '/': {
        lookahead_char = currString[++lookahead];
        token = Token::DIVIDE;
    } break;
    case '<':
    {
        lookahead_char = currString[++lookahead];
        if (lookahead < currString.size()) {
            if (lookahead_char == '=') {
                lookahead++;
                token = Token::LESS_EQ;
                break;
            }
            if (lookahead_char == '>') {
                lookahead++;
                token = Token::NOT_EQUALS;
                break;
            }
        }
        token = Token::LESS;
    } break;
    case '>':
    {
        lookahead_char = currString[++lookahead];
        if (lookahead < currString.size()) {
            if (lookahead_char == '=') {
                lookahead++;
                token = Token::GREATER_EQ;
                break;
            }
        }
        token = Token::GREATER;
    } break;
    case '=':
    {
        lookahead_char = currString[++lookahead];
        token = Token::EQUALS;
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
        while (_isAlphaNumerical(lookahead_char) && lookahead < currString.size()) {
            lookahead_char = currString[++lookahead];
        }
        token = Token::ID;
    } break;
    }
    return std::make_pair<>(lookahead, token);
}

void Lexer::LoadInputString(const std::string& string) {
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