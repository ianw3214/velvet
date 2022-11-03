#include "parser.h"

#include <iostream>

#include "lexer/lexer.h"

namespace {
    bool _isRelationalOperator(Token token) {
        constexpr int start_index = static_cast<int>(Token::GREATER);
        constexpr int end_index = static_cast<int>(Token::NOT_EQUALS);
        const int target = static_cast<int>(token);
        return target >= start_index && target <= end_index;
    }

    bool _isBinaryOperator(Token token) {
        constexpr int start_index = static_cast<int>(Token::PLUS);
        constexpr int end_index = static_cast<int>(Token::MINUS);
        const int target = static_cast<int>(token);
        return target >= start_index && target <= end_index;
    }
}

ASTNode* Parser::Parse() {
    return ParseStatementList();
    // return ParseExpr();
}

ASTNode* Parser::ParseStatementList() {
    ASTNode* statement = ParseStatement();
    if (!statement) {
        return nullptr;
    }
    ASTNode* statement_opt = nullptr;
    Lexeme lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::VAR_DECL || lexeme.token == Token::ID) {
        statement_opt = ParseStatementList();
    }
    std::vector<ASTNode*> statements;
    statements.push_back(statement);
    if (StatementListNode* list = dynamic_cast<StatementListNode*>(statement_opt)) {
        statements.insert(statements.end(), list->mStatements.begin(), list->mStatements.end());
    }
    return new StatementListNode(std::move(statements));
}

ASTNode* Parser::ParseStatement() {
    Lexeme lexeme = Lexer::peekLexeme();
    switch (lexeme.token) {
    case Token::FN_DECL: {
        return ParseFunctionDeclaration();
    } break;
    case Token::VAR_DECL: {
        return ParseVariableDeclaration();
    } break;
    case Token::ID: {
        return ParseAssignmentStmt();
    } break;
    }
    std::cout << "Error! Expected valid statement\n";
    return nullptr;
}

ASTNode* Parser::ParseFunctionDeclaration() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::FN_DECL) {
        std::cout << "Error! Expected function declaration(var)\n";
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "Error! Expected id token\n";
    }
    std::string identifier = lexeme.symbol;
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::LEFT_BRACKET) {
        std::cout << "Error! Expected left bracket\n";
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::RIGHT_BRACKET) {
        std::cout << "Error! Expected right bracket\n";
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::FN_TYPE_RESULT) {
        std::cout << "Error! Expected -> for function type result\n";
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "Error! Expected id token for function type\n";
    }
    std::string type = lexeme.symbol;
    ASTNode* block = ParseBlockExpr();
    return new FunctionDeclNode(identifier, type, block);
}

ASTNode* Parser::ParseVariableDeclaration() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::VAR_DECL) {
        std::cout << "Error! Expected variable declaration (var)\n";
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "Error! Expected id token\n";
    }
    std::string identifier = lexeme.symbol;
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::TYPE_DECL) {
        std::cout << "Error! Expected type declaration ($)\n";
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "Error! Expected type name id token\n";
    }
    std::string type = lexeme.symbol;
    // optional assignment after var declaration
    ASTNode* opt_assign = nullptr;
    lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::ASSIGNMENT) {
        Lexer::getLexeme();
        opt_assign = ParseExpr();
    }
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::STATEMENT_END) {
        std::cout << "Error! Expected end of statement token (;)\n";
        return nullptr;
    }
    return new VariableDeclarationNode(identifier, type, opt_assign);
}

ASTNode* Parser::ParseAssignmentStmt() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ID) {
        std::cout << "ERROR! Expected id token\n";
        return nullptr;
    }
    std::string identifier = lexeme.symbol;
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::ASSIGNMENT) {
        std::cout << "ERROR! Expected assignment operator (:=) \n";
        return nullptr;
    }
    ASTNode* expr = ParseExpr();
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::STATEMENT_END) {
        std::cout << "Error! Expected end of statement token (;)\n";
        return nullptr;
    }
    return new AssignmentStatementNode(identifier, expr);
}

ASTNode* Parser::ParseExpr() {
    Lexeme lexeme = Lexer::peekLexeme();
    switch (lexeme.token) {
    case Token::LEFT_CURLY_BRACKET: {
        return ParseBlockExpr();
    } break;
    case Token::IF: {
        return ParseIfExpr();
    } break;
    case Token::LOOP: {
        return ParseLoopExpr();
    } break;
    default: {
        return ParseRelExpr();
    }
    }
    return nullptr;
}

ASTNode* Parser::ParseBlockExpr() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::LEFT_CURLY_BRACKET) {
        std::cout << "ERROR! Expected left curly bracket\n";
    }
    // If we fail to find a statement, backtrack and parse an expression instead
    Lexer::SetBacktrackPoint();
    ASTNode* statementList = ParseStatementList();
    if (!statementList) {
        Lexer::JumpToBacktrackPoint();
    }
    ASTNode* expr = ParseExpr();
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::RIGHT_CURLY_BRACKET) {
        std::cout << "ERROR! Expected right curly bracket\n";
    }
    return new BlockExpressionNode(statementList, expr);
}

ASTNode* Parser::ParseIfExpr() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::IF) {
        std::cout << "ERROR! Expected 'if'\n";
    }
    ASTNode* condition = ParseExpr();
    lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::THEN) {
        std::cout << "ERROR! Expected 'then'\n";
    }
    ASTNode* then = ParseExpr();
    ASTNode* opt_else = nullptr;
    lexeme = Lexer::peekLexeme();
    if (lexeme.token == Token::ELSE) {
        Lexer::getLexeme();
        opt_else = ParseExpr();
    }
    return new IfExpressionNode(condition, then, opt_else);
}

ASTNode* Parser::ParseLoopExpr() {
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token != Token::LOOP) {
        std::cout << "ERROR! Expected 'loop'\n";
    }
    ASTNode* block = ParseBlockExpr();
    return new LoopExpressionNode(block);
}

ASTNode* Parser::ParseRelExpr() {
    ASTNode* lhs = ParseBinExpr();
    Lexeme lexeme = Lexer::peekLexeme();
    if (_isRelationalOperator(lexeme.token)) {
        Lexer::getLexeme();
        ASTNode* rhs = ParseBinExpr();
        return new RelationalOperatorNode(lhs, rhs, lexeme.token);
    }
    return lhs;
}

ASTNode* Parser::ParseBinExpr() {
    ASTNode* term = ParseTerm();
    ASTNode* opt_rhs = ParseBinExprRHS(term);
    return opt_rhs ? opt_rhs : term;
}

ASTNode* Parser::ParseBinExprRHS(ASTNode* left){
    Lexeme lexeme = Lexer::peekLexeme();
    if (_isBinaryOperator(lexeme.token)) {
        Lexer::getLexeme();
        ASTNode* right = ParseExpr();
        ASTNode* binop = new BinaryOperatorNode(left, right, lexeme.token);
        ASTNode* opt_post = ParseBinExprRHS(binop);
        return opt_post ? opt_post : binop;
    }
    return nullptr;
}

ASTNode* Parser::ParseTerm(){
    Lexeme lexeme = Lexer::getLexeme();
    if (lexeme.token == Token::LEFT_BRACKET) {
        ASTNode* expr = ParseExpr();
        lexeme = Lexer::getLexeme();
        if (lexeme.token != Token::RIGHT_BRACKET) {
            std::cout << "ERROR! Expected ')'\n";
        }
        return expr;
    }
    if (lexeme.token == Token::ID) {
        return new IdentifierNode(lexeme.symbol);
    }
    if (lexeme.token == Token::NUM) {
        return new NumberNode(lexeme.symbol);
    }
    // I think this should error?
    return nullptr;
}
