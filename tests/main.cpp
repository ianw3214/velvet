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

TEST_CASE("Newline lexes correctly as white space", "[lexer]") {
	_verifyInputStringGeneratesTokens("line1 \n line2", { Token::ID, Token::ID });
}

TEST_CASE("Tab lexes correctly as white space", "[lexer]") {
	_verifyInputStringGeneratesTokens("chunk1 \t chunk2", { Token::ID, Token::ID });
}

TEST_CASE("Commas lex correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("a, b, c", { Token::ID, Token::COMMA, Token::ID, Token::COMMA, Token::ID });
}

TEST_CASE("Commas with no space lex correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("a,b,c", { Token::ID, Token::COMMA, Token::ID, Token::COMMA, Token::ID });
}

TEST_CASE("Numbers with decimals lex correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("11.11, 22.22, 33.33", { Token::NUM, Token::COMMA, Token::NUM, Token::COMMA, Token::NUM });
}

TEST_CASE("Expressions with array access lex correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("a[0] + b[1]", { Token::ID, Token::LEFT_SQUARE_BRACKET, Token::NUM, Token::RIGHT_SQUARE_BRACKET, Token::PLUS, Token::ID, Token::LEFT_SQUARE_BRACKET, Token::NUM, Token::RIGHT_SQUARE_BRACKET });
}

TEST_CASE("Expression with space in array access lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("a [ test ]", { Token::ID, Token::LEFT_SQUARE_BRACKET, Token::ID, Token::RIGHT_SQUARE_BRACKET });
}

TEST_CASE("Return keyword lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("return identifier;", { Token::RETURN, Token::ID, Token::EXPRESSION_END });
}

TEST_CASE("Break keyword lexes correctly", "[lexer]") {
	_verifyInputStringGeneratesTokens("something; break;", { Token::ID, Token::EXPRESSION_END, Token::BREAK, Token::EXPRESSION_END });
}

TEST_CASE("GetLexeme lexes correctly after peeking ahead by 1", "[lexer]") {
	Lexer::LoadInputString("a");
	Lexeme peekLexeme = Lexer::peekLexeme();
	REQUIRE(peekLexeme.token == Token::ID);
	REQUIRE(peekLexeme.symbol == "a");
	Lexeme getLexeme = Lexer::getLexeme();
	REQUIRE(getLexeme.token == Token::ID);
	REQUIRE(getLexeme.symbol == "a");
}

TEST_CASE("GetLexeme lexes correctly after peeking ahead by 2", "[lexer]") {
	Lexer::LoadInputString("a b");
	Lexeme peekLexeme = Lexer::peekLexeme(2);
	REQUIRE(peekLexeme.token == Token::ID);
	REQUIRE(peekLexeme.symbol == "b");
	Lexeme getLexeme1 = Lexer::getLexeme();
	REQUIRE(getLexeme1.token == Token::ID);
	REQUIRE(getLexeme1.symbol == "a");
	Lexeme getLexeme2 = Lexer::getLexeme();
	REQUIRE(getLexeme2.token == Token::ID);
	REQUIRE(getLexeme2.symbol == "b");
}

TEST_CASE("GetLexeme peeks correctly out of order", "[lexer]") {
	Lexer::LoadInputString("a b");
	Lexeme peekLexeme2 = Lexer::peekLexeme(2);
	REQUIRE(peekLexeme2.token == Token::ID);
	REQUIRE(peekLexeme2.symbol == "b");
	Lexeme peekLexeme1 = Lexer::peekLexeme();
	REQUIRE(peekLexeme1.token == Token::ID);
	REQUIRE(peekLexeme1.symbol == "a");
}

TEST_CASE("GetLexeme peeks correctly after repeated uses", "[lexer]") {
	// TODO: Maybe this should be shared w/ actual lexer code somehow
	constexpr int LEXER_WINDOW_SIZE = 5;
	Lexer::LoadInputString("0 1 2 3 4 5 6 7 8 9 10");
	int currNum = 0;
	while (currNum <= 10) {
		Lexeme lexeme = Lexer::peekLexeme();
		REQUIRE(lexeme.token == Token::NUM);
		REQUIRE(lexeme.symbol == std::to_string(currNum));
		int lastNum = std::min(currNum + LEXER_WINDOW_SIZE, 10);
		for (int num = currNum; num <= lastNum; num++) {
			Lexeme peekLexeme = Lexer::peekLexeme(num - currNum + 1);
			REQUIRE(peekLexeme.token == Token::NUM);
			REQUIRE(peekLexeme.symbol == std::to_string(num));
		}
		Lexer::getLexeme();
		currNum++;
	}
}