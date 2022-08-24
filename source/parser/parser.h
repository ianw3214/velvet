#pragma once

#include "common.h"

#include <functional>

namespace Parser {
	void Parse(std::function<Lexeme()> getTokenCallback);

    void ParseExpr(std::function<Lexeme()> getTokenCallback);
    void ParseExprPost(std::function<Lexeme()> getTokenCallback);
    void ParseTerm(std::function<Lexeme()> getTokenCallback);
}