#include <iostream>
#include "llvm/IR/LLVMContext.h"

#include <string>
#include <unordered_map>
#include <vector>

#pragma optimize("", off)

struct DFAState {
    int defaultState;
    std::unordered_map<char, int> transitions;
};

std::unordered_map<int, DFAState> DFA;

enum class Token{
    INVALID = -1,

    WHITESPACE,

    // keywords
    IF,
    THEN,
    ELSE,

    // identifiers/literals
    ID,
    NUM,

    // operators
    RELOP
};

std::unordered_map<int, Token> acceptingStates;

Token getLexeme(const std::string& string) {
    int curr_state = 0;
    int last_accepting = -1;
    int curr_char_index = 0;
    while (curr_state != -1 && curr_char_index < string.size()) {
        if (acceptingStates.find(curr_state) != acceptingStates.end()) {
            last_accepting = curr_state;
        }
        char curr_char = string[curr_char_index];
        auto it = DFA[curr_state].transitions.find(curr_char);
        if (it == DFA[curr_state].transitions.end()) {
            curr_state = DFA[curr_state].defaultState;
        }
        else {
            curr_state = it->second;
        }
        curr_char_index++;
    }
    if (acceptingStates.find(curr_state) != acceptingStates.end()) {
        last_accepting = curr_state;
    }
    if (last_accepting != -1) {
        return acceptingStates[last_accepting];
    }
    return Token::INVALID;
}

int main() {
    llvm::LLVMContext context;
    std::cout << &context << std::endl;

    // initilaize the DFA
    DFA[-1] = {};
    DFA[0] = { 12 };
    DFA[0].transitions[' '] = 1;
    DFA[0].transitions['\n'] = 1;
    DFA[0].transitions['\t'] = 1;
    DFA[0].transitions['i'] = 2;
    DFA[0].transitions['t'] = 4;
    DFA[0].transitions['e'] = 8;
    DFA[1] = { -1 };
    DFA[1].transitions[' '] = 1;
    DFA[1].transitions['\n'] = 1;
    DFA[1].transitions['\t'] = 1;
    DFA[2] = { 12 };
    DFA[2].transitions['f'] = 3;
    DFA[3] = { 12 };
    DFA[3].transitions[' '] = -1;
    DFA[3].transitions['\n'] = -1;
    DFA[3].transitions['\t'] = -1;
    DFA[4] = { 12 };
    DFA[4].transitions['h'] = 5;
    DFA[5] = { 12 };
    DFA[5].transitions['e'] = 6;
    DFA[6] = { 12 };
    DFA[6].transitions['n'] = 7;
    DFA[7] = { 12 };
    DFA[7].transitions[' '] = -1;
    DFA[7].transitions['\n'] = -1;
    DFA[7].transitions['\t'] = -1;
    DFA[8] = { 12 };
    DFA[8].transitions['l'] = 9;
    DFA[9] = { 12 };
    DFA[9].transitions['s'] = 10;
    DFA[10] = { 12 };
    DFA[10].transitions['e'] = 11;
    DFA[11] = { 12 };
    DFA[11].transitions[' '] = -1;
    DFA[11].transitions['\n'] = -1;
    DFA[11].transitions['\t'] = -1;
    DFA[12] = { 12 };
    DFA[13] = { -1 };

    acceptingStates = {
        { 1, Token::WHITESPACE },
        { 3, Token::IF },
        { 7, Token::THEN },
        { 11, Token::ELSE },
        { 12, Token::ID },
        { 13, Token::NUM }
    };

    std::string test_string = "else testID otherstuff";
    Token lexeme = getLexeme(test_string);

    if (lexeme == Token::INVALID) {
        std::cout << "AH FUCK\n";
    }
    if (lexeme == Token::WHITESPACE) {
        std::cout << "WHITESPACE\n";
    }
    if (lexeme == Token::IF) {
        std::cout << "IF\n";
    }
    if (lexeme == Token::THEN) {
        std::cout << "THEN\n";
    }
    if (lexeme == Token::ELSE) {
        std::cout << "ELSE\n";
    }
    if (lexeme == Token::ID) {
        std::cout << "ID\n";
    }
    if (lexeme == Token::NUM) {
        std::cout << "NUM\n";
    }

    return 0;
};