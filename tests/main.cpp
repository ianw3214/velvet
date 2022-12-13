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

TEST_CASE("Verify types lex correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("i32, f32, bool", { Token::TYPE_I32, Token::COMMA, Token::TYPE_F32, Token::COMMA, Token::TYPE_BOOL });
}

TEST_CASE("If statement lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("if cond then expr1 else expr2", { Token::IF, Token::ID, Token::THEN, Token::ID, Token::ELSE, Token::ID });
}

TEST_CASE("Variable declaration statement lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("var test $ i32 := something", { Token::VAR_DECL, Token::ID, Token::TYPE_DECL, Token::TYPE_I32, Token::ASSIGNMENT, Token::ID });
}

TEST_CASE("Statement list with seperators lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("var test $ f32; test := expr;", { Token::VAR_DECL, Token::ID, Token::TYPE_DECL, Token::TYPE_F32, Token::EXPRESSION_END, Token::ID, Token::ASSIGNMENT, Token::ID, Token::EXPRESSION_END });
}

TEST_CASE("Function declaration with block lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("fn foo() {}", { Token::FN_DECL, Token::ID, Token::LEFT_BRACKET, Token::RIGHT_BRACKET, Token::LEFT_CURLY_BRACKET, Token::RIGHT_CURLY_BRACKET });
}

TEST_CASE("Loop expression lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("loop { foo() }", { Token::LOOP, Token::LEFT_CURLY_BRACKET, Token::ID, Token::LEFT_BRACKET, Token::RIGHT_BRACKET, Token::RIGHT_CURLY_BRACKET });
}

TEST_CASE("Commas lex correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("a, b, c", { Token::ID, Token::COMMA, Token::ID, Token::COMMA, Token::ID });
}

TEST_CASE("Numbers with decimals lex correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("11.11, 22.22, 33.33", { Token::NUM, Token::COMMA, Token::NUM, Token::COMMA, Token::NUM });
}
