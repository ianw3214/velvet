#pragma once

#include "common.h"

#include <functional>

namespace Parser {
	void Parse();

    void ParseExpr();
    void ParseExprPost();
    void ParseTerm();
}