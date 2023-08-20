#pragma once

#include <string>
#include <vector>
#include <utility>

#include "lexer/tokens.h"

class Lexer {
    std::vector<std::pair<Token, std::string>> mTokens;
    size_t mCurrTokenIndex;
public:
    Lexer(std::string input);

    Token getCurrToken() const;
    const std::string& getCurrTokenStr() const;
    void consumeToken();
private:
    void checkAndAddSingleToken(char c);
};