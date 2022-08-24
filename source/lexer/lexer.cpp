#include "lexer.h"

namespace {
    struct DFAState {
        int defaultState;
        std::unordered_map<char, int> transitions;

        void addWhitespaceTransitionTo(int next_state) {
            transitions[' '] = next_state;
            transitions['\n'] = next_state;
            transitions['\t'] = next_state;
            transitions['\r'] = next_state;
        }

        void addNumericTransitionTo(int next_state) {
            for (int i = 0; i < 10; ++i) {
                transitions['0' + i] = next_state;
            }
        }
    };

    std::unordered_map<int, DFAState> DFA;
    std::unordered_map<int, Token> acceptingStates;

    // Current string being analyzed
    std::string currString;
    int curr_index = 0;
}

void Lexer::Initialize() {
    // initilaize the DFA
    DFA[-1] = {};
    DFA[0] = { 12 };
    DFA[0].addWhitespaceTransitionTo(1);
    DFA[0].addNumericTransitionTo(13);
    DFA[0].transitions['i'] = 2;
    DFA[0].transitions['t'] = 4;
    DFA[0].transitions['e'] = 8;
    DFA[1] = { -1 };
    DFA[1].addWhitespaceTransitionTo(1);
    DFA[2] = { 12 };
    DFA[2].transitions['f'] = 3;
    DFA[3] = { 12 };
    DFA[3].addWhitespaceTransitionTo(-1);
    DFA[4] = { 12 };
    DFA[4].transitions['h'] = 5;
    DFA[5] = { 12 };
    DFA[5].transitions['e'] = 6;
    DFA[6] = { 12 };
    DFA[6].transitions['n'] = 7;
    DFA[7] = { 12 };
    DFA[7].addWhitespaceTransitionTo(-1);
    DFA[8] = { 12 };
    DFA[8].transitions['l'] = 9;
    DFA[9] = { 12 };
    DFA[9].transitions['s'] = 10;
    DFA[10] = { 12 };
    DFA[10].transitions['e'] = 11;
    DFA[11] = { 12 };
    DFA[11].addWhitespaceTransitionTo(-1);
    DFA[12] = { 12 };
    DFA[12].addWhitespaceTransitionTo(-1);
    DFA[13] = { -1 };
    DFA[13].addNumericTransitionTo(13);

    acceptingStates = {
        { 1, Token::WHITESPACE },
        { 3, Token::IF },
        { 7, Token::THEN },
        { 11, Token::ELSE },
        { 12, Token::ID },
        { 13, Token::NUM }
    };
}

void Lexer::LoadString(const std::string& string) {
    currString = string;
    curr_index = 0;
}

Lexeme Lexer::getLexeme() {
    int curr_state = 0;
    int last_accepting = -1;
    int last_accepting_index = -1;
    int lookahead = curr_index;
    while (curr_state != -1 && lookahead < currString.size()) {
        if (acceptingStates.find(curr_state) != acceptingStates.end()) {
            last_accepting = curr_state;
            last_accepting_index = lookahead;
        }
        char curr_char = currString[lookahead];
        auto it = DFA[curr_state].transitions.find(curr_char);
        if (it == DFA[curr_state].transitions.end()) {
            curr_state = DFA[curr_state].defaultState;
        }
        else {
            curr_state = it->second;
        }
        lookahead++;
    }
    if (acceptingStates.find(curr_state) != acceptingStates.end()) {
        last_accepting = curr_state;
        last_accepting_index = lookahead;
    }
    Lexeme result = { Token::INVALID };
    if (last_accepting != -1) {
        size_t size = static_cast<size_t>(lookahead - curr_index) - (lookahead == currString.size() ? 0 : 1);
        result.symbol = currString.substr(curr_index, size);
        result.token = acceptingStates[last_accepting];
        curr_index = last_accepting_index;
    }
    return result;
}