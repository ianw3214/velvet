#pragma once

#include "common.h"

#include <string>
#include <unordered_map>
#include <vector>

#include<sstream>
#include <fstream>

/** Read file into string. */
inline std::string slurp(const std::string& path) {
	std::ostringstream buf;
	std::ifstream input(path.c_str());
	buf << input.rdbuf();
	return buf.str();
}

namespace Lexer {
	struct Lexeme {
		Token token;
		std::string symbol;
	};

	void Initialize();
	void LoadString(const std::string& string);
	Lexeme getLexeme();
}