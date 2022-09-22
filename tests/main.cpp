#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <string>
#include <vector>

#include "lexer/lexer.h"

namespace {
	void _verifyInputStringGeneratesTokens(const std::string& inputString, const std::vector<Token> tokens) {
		Lexer::LoadInputString(inputString);
		for (Token token : tokens) {
			Lexeme lexeme = Lexer::getLexeme();
			REQUIRE(lexeme.token == token);
		}
	}
}

TEST_CASE("Relational operators with numerical literals lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("a > 100", { Token::ID, Token::GREATER, Token::NUM });
}

TEST_CASE("Binary operators with brackets lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("a + (b - c)", { Token::ID, Token::PLUS, Token::LEFT_BRACKET, Token::ID, Token::MINUS, Token::ID, Token::RIGHT_BRACKET });
}

TEST_CASE("If statement lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("if cond then expr1 else expr2", { Token::IF, Token::ID, Token::THEN, Token::ID, Token::ELSE, Token::ID });
}