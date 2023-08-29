#include "lexer.h"

#include <unordered_map>

namespace {
    enum class LexType {
        NONE,
        ID,
        NUM,
        SYMBOL,
        COMMENT
    };

    inline bool _isAlphabet(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    inline bool _isNumeric(char c) {
        return c >= '0' && c <= '9';
    }

    inline bool _isAlphaNumeric(char c) {
        return _isAlphabet(c) || (c >= 'a' && c <= 'z') || _isNumeric(c);
    }

    inline bool _isValidIdentifierStart(char c) {
        return _isAlphabet(c) || c == '_';
    }

    inline bool _isValidIdentifierChar(char c) {
        return _isAlphaNumeric(c) || c == '_';
    }

    inline bool _isUniqueSymbol(char c) {
        return c == '=' || c == '!' || c == '>' || c == '<';
    }

    const std::unordered_map<std::string, Token> keywordMap = {
        { "def", Token::FUNC_DEF },
        { "var", Token::VAR_DEF },
        { "if", Token::IF },
        { "then", Token::THEN },
        { "else", Token::ELSE },
        { "loop", Token::LOOP },
        { "break", Token::BREAK },
        { "arrdecay", Token::ARRAY_DECAY },
        // types
        { "i32", Token::TYPE_I32 },
        { "f32", Token::TYPE_F32 },
        { "bool", Token::TYPE_BOOL }
    };

    const std::unordered_map<char, Token> singleCharMap = {
        { ',', Token::COMMA },
        { ':', Token::COLON },
        { ';', Token::SEMICOLON },
        { '(', Token::LEFT_PARENTHESIS },
        { ')', Token::RIGHT_PARENTHESIS },
        { '{', Token::LEFT_BRACKET },
        { '}', Token::RIGHT_BRACKET },
        { '[', Token::LEFT_SQUARE_BRACKET },
        { ']', Token::RIGHT_SQUARE_BRACKET },
        { '@', Token::FUNC_RETURN },
        { '+', Token::PLUS },
        { '-', Token::MINUS },
        { '*', Token::MULTIPLY },
        { '/', Token::DIVIDE }
    };

    const std::unordered_map<std::string, Token> symbolMap = {
        { "=", Token::ASSIGN },
        { "==", Token::EQUALS },
        { "!=", Token::NOT_EQUALS },
        { ">", Token::GREATER },
        { ">=", Token::GREATER_EQUALS},
        { "<", Token::LESS },
        { "<=", Token::LESS_EQUALS}
    };

    constexpr char commentSymbol = '#';
    constexpr char commentEnd = '\n';
}

Lexer::Lexer(std::string input) 
    : mTokens() 
    , mCurrTokenIndex(0) 
{
    LexType currLexType = LexType::NONE;
    std::string currToken = "";
    for (const char c : input) {
        if (c == commentSymbol) {
            currLexType = LexType::COMMENT;
        }
        if (currLexType == LexType::NONE) {
            if (_isValidIdentifierStart(c)) {
                currLexType = LexType::ID;
                currToken += c;
            }
            else if (_isNumeric(c) || c == '.') {
                currLexType = LexType::NUM;
                currToken += c;
            }
            else if (_isUniqueSymbol(c)) {
                currLexType = LexType::SYMBOL;
                currToken += c;
            } else {
                checkAndAddSingleToken(c);
            }
        }
        else if (currLexType == LexType::ID) {
            if (_isValidIdentifierChar(c)) {
                currToken += c;
            }
            else {
                auto it = keywordMap.find(currToken);
                if (it != keywordMap.end()) {
                    mTokens.emplace_back(it->second, currToken);
                }
                else {
                    mTokens.emplace_back(Token::ID, currToken);
                }
                currToken.clear();
                currLexType = LexType::NONE;
                checkAndAddSingleToken(c);
            }
        }
        else if (currLexType == LexType::NUM) {
            if (_isNumeric(c) || c == '.') {
                currToken += c;
            }
            else {
                mTokens.emplace_back(Token::NUM, currToken);
                currToken.clear();
                currLexType = LexType::NONE;
                checkAndAddSingleToken(c);
            }
        }
        else if (currLexType == LexType::SYMBOL) {
            if (_isUniqueSymbol(c)) {
                currToken += c;
            }
            else {
                auto it = symbolMap.find(currToken);
                if (it != symbolMap.end()) {
                    mTokens.emplace_back(it->second, currToken);
                }
                else {
                    // TODO: Error?
                }
                currToken.clear();
                if (_isAlphabet(c)) {
                    currLexType = LexType::ID;
                    currToken += c;
                }
                else if (_isNumeric(c)) {
                    currLexType = LexType::NUM;
                    currToken += c;
                } else {
                    checkAndAddSingleToken(c);
                }
            }
        }
        else if (currLexType == LexType::COMMENT) {
            if (c == '\n') {
                currLexType = LexType::NONE;
            }
        }
    }
    if (currToken.length() > 0) {
        if (currLexType == LexType::ID) {
            mTokens.emplace_back(Token::ID, currToken);
        }
        if (currLexType == LexType::NUM) {
            mTokens.emplace_back(Token::NUM, currToken);
        }
    }
    mTokens.emplace_back(Token::TOK_EOF, "");
}

Token Lexer::getCurrToken() const {
    return mTokens[mCurrTokenIndex].first;
}

const std::string& Lexer::getCurrTokenStr() const {
    return mTokens[mCurrTokenIndex].second;
}

void Lexer::consumeToken() {
    mCurrTokenIndex++;
}

void Lexer::checkAndAddSingleToken(char c) {
    auto it = singleCharMap.find(c);
    if (it != singleCharMap.end()) {
        mTokens.emplace_back(it->second, std::string(1, c));
    }
}