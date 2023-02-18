#include "lexer.h"

#include <unordered_map>
#include <utility>
#include <array>
    
namespace {
    // Current window of strings being analyzed
    constexpr size_t WINDOW_SIZE = 5;
    std::array<Token, WINDOW_SIZE> tokenBuffer;
    // TODO: Use string views?
    std::array<std::string, WINDOW_SIZE> stringBuffer;
    size_t currToken = 0;
    size_t currentWindow = 0;

    // Current string being analyzed
    std::string inputString;
    int curr_index = 0;

    std::unordered_map<std::string, Token> keywords = {
        { "if", Token::IF },
        { "then", Token::THEN },
        { "else", Token::ELSE },
        { "loop", Token::LOOP },
        { "var", Token::VAR_DECL },
        { "fn", Token::FN_DECL },
        // types
        { "i32" , Token::TYPE_I32 },
        { "f32" , Token::TYPE_F32 },
        { "bool" , Token::TYPE_BOOL },
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
    if (lookahead == inputString.size()) {
        return std::make_pair<>(lookahead, Token::TOKEN_EOF);
    }
    char lookahead_char = inputString[lookahead];
    switch (lookahead_char) {
    case ' ':
    case '\t':
    case '\n':
    {
        while ((lookahead_char == ' ' || lookahead_char == '\t' || lookahead_char == '\n') && lookahead < inputString.size()) {
            lookahead_char = inputString[++lookahead];
        }
        token = Token::WHITESPACE;
    } break;
    case ';': {
        lookahead_char = inputString[++lookahead];
        token = Token::EXPRESSION_END;
    } break;
    case ',': {
        lookahead_char = inputString[++lookahead];
        token = Token::COMMA;
    } break;
    case ':': {
        lookahead_char = inputString[++lookahead];
        if (lookahead < inputString.size()) {
            if (lookahead_char == '=') {
                lookahead++;
                token = Token::ASSIGNMENT;
                break;
            }
        }
    } break;
    case '$': {
        lookahead_char = inputString[++lookahead];
        token = Token::TYPE_DECL;
    } break;
    case '(': {
        lookahead_char = inputString[++lookahead];
        token = Token::LEFT_BRACKET;
    } break;
    case ')': {
        lookahead_char = inputString[++lookahead];
        token = Token::RIGHT_BRACKET;
    } break;
    case '{': {
        lookahead_char = inputString[++lookahead];
        token = Token::LEFT_CURLY_BRACKET;
    } break;
    case '}': {
        lookahead_char = inputString[++lookahead];
        token = Token::RIGHT_CURLY_BRACKET;
    } break;
    case '+': {
        lookahead_char = inputString[++lookahead];
        token = Token::PLUS;
    } break;
    case '-': {
        lookahead_char = inputString[++lookahead];
        if (lookahead < inputString.size()) {
            if (lookahead_char == '>') {
                lookahead++;
                token = Token::FN_TYPE_RESULT;
                break;
            }
        }
        token = Token::MINUS;
    } break;
    case '*': {
        lookahead_char = inputString[++lookahead];
        token = Token::MULTIPLY;
    } break;
    case '/': {
        lookahead_char = inputString[++lookahead];
        token = Token::DIVIDE;
    } break;
    case '<':
    {
        lookahead_char = inputString[++lookahead];
        if (lookahead < inputString.size()) {
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
        lookahead_char = inputString[++lookahead];
        if (lookahead < inputString.size()) {
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
        lookahead_char = inputString[++lookahead];
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
        while (((lookahead_char >= '0' && lookahead_char <= '9') || lookahead_char == '.') && lookahead < inputString.size()) {
            lookahead_char = inputString[++lookahead];
        }
        token = Token::NUM;
    } break;
    default:
    {
        while (_isAlphaNumerical(lookahead_char) && lookahead < inputString.size()) {
            lookahead_char = inputString[++lookahead];
        }
        token = Token::ID;
    } break;
    }
    return std::make_pair<>(lookahead, token);
}

void Lexer::LoadInputString(const std::string& string) {
    inputString = string;
    curr_index = 0;
}

Lexeme Lexer::peekLexeme(int peekahead) {
    // First verify that we are peaking within a valid window size
    if (peekahead > WINDOW_SIZE || peekahead < 1) {
        // TODO: Assert
    }

    size_t targetIndex = currToken + peekahead - 1;
    targetIndex -= targetIndex >= WINDOW_SIZE ? WINDOW_SIZE : 0;
    size_t nextIndex = currToken + currentWindow;

    // Skip the tokens that we've already peeked
    int numTokensToLex = peekahead - currentWindow;
    currentWindow += numTokensToLex;
    // TODO: Assert that this is greater than 0

    while (numTokensToLex > 0) {
        std::pair<int, Token> new_lookahead = _advanceLookahead(curr_index);

        Lexeme result;
        size_t size = static_cast<size_t>(new_lookahead.first - curr_index);
        result.symbol = inputString.substr(curr_index, size);
        result.token = new_lookahead.second;
        curr_index = new_lookahead.first;

        if (result.token == Token::WHITESPACE) {
            int token_start = new_lookahead.first;
            new_lookahead = _advanceLookahead(new_lookahead.first);

            size_t size = static_cast<size_t>(new_lookahead.first - token_start);
            result.symbol = inputString.substr(token_start, size);
            result.token = new_lookahead.second;
            curr_index = new_lookahead.first;
        }

        if (result.token == Token::ID) {
            auto entry = keywords.find(result.symbol);
            if (entry != keywords.end()) {
                result.token = entry->second;
            }
        }

        // return result;
        nextIndex -= nextIndex >= WINDOW_SIZE ? WINDOW_SIZE : 0;
        tokenBuffer[nextIndex] = result.token;
        stringBuffer[nextIndex] = result.symbol;
        numTokensToLex--;
        nextIndex++;
    }

    const Token token = tokenBuffer[targetIndex];
    const std::string string = stringBuffer[targetIndex];
    return Lexeme{ token, string };
}

Lexeme Lexer::getLexeme() {
    // If the window size is greater than 1, then we have already peeked ahead
    if (currentWindow > 0) {
        const Token token = tokenBuffer[currToken];
        const std::string str = stringBuffer[currToken];
        currToken++;
        currToken -= currToken >= WINDOW_SIZE ? WINDOW_SIZE : 0;
        currentWindow--;
        return Lexeme{ token, str };
    }
    // If the window size is 0, we don't need to mess with it
    //  since it will be read and then discarded by the lexer

    std::pair<int, Token> new_lookahead = _advanceLookahead(curr_index);

    Lexeme result;
    size_t size = static_cast<size_t>(new_lookahead.first - curr_index);
    result.symbol = inputString.substr(curr_index, size);
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