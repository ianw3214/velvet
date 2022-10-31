#pragma once

#include "common.h"

#include <string>

#include <sstream>
#include <fstream>

/** Read file into string. */
inline std::string slurp(const std::string& path) {
	std::ostringstream buf;
	std::ifstream input(path.c_str());
	buf << input.rdbuf();
	return buf.str();
}

namespace Lexer {
	void LoadInputString(const std::string& string);
	Lexeme peekLexeme();
	Lexeme getLexeme();

	void SetBacktrackPoint();
	void JumpToBacktrackPoint();
}