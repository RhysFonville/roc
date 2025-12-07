#include "Parser.h"
#include "Lexer.h"
#include "Syntax.h"

std::vector<std::shared_ptr<Statement>> Parser::run() {
	std::vector<std::shared_ptr<Statement>> statements{};
	while (!is_at_end()) {
		try {
			statements.push_back(statement());
		} catch (const ParserException& e) {
			synchronize();
			return {};
		}
	}
	return statements;
}

ParserException Parser::parse_error(const Token& tok, const std::string& message) const noexcept {
	error(tok, message);
	return ParserException{tok, message};
}

Token Parser::peek() const noexcept {
	return toks[current];
}

bool Parser::is_at_end() const noexcept {
	return (peek().type == TokenType::END_OF_FILE);
}

bool Parser::check(TokenType type) const noexcept {
	if (is_at_end()) return false;
	return (peek().type == type);
}

bool Parser::check(const std::vector<TokenType>& types) const noexcept {
	if (is_at_end()) return false;

	for (const auto& type : types) {
		if (type == peek().type) return true;
	}
	return false;
}

Token Parser::previous() const noexcept {
	return toks[current-1];
}

Token Parser::advance() noexcept{
	if (!is_at_end()) current++;
	return previous();
}

bool Parser::match(TokenType type) noexcept {
	if (check(type)) {
		advance();
		return true;
	}

	return false;
}

bool Parser::match(const std::vector<TokenType>& types) noexcept {
	for (TokenType type : types) {
		if (check(type)) {
			advance();
			return true;
		}
	}

	return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
	if (check(type)) return advance();
	throw parse_error(peek(), message);
	return Token{};
}

Token Parser::consume(const std::vector<TokenType>& types, const std::string& message) {
	for (const auto& type : types)
		if (check(type)) return advance();
	throw parse_error(peek(), message);
	return Token{};
}

void Parser::synchronize() noexcept {
	advance();
	while (!is_at_end()) {
		if (previous().type == TokenType::SEMICOLON) return;
		advance();
	}
}

std::shared_ptr<Expression> Parser::primary_expression() {
	if (check(TokenType::IDENTIFIER)) {
		return std::make_shared<IdentifierExpression>(advance());
	} else if (check(literal_tokens)) {
		return std::make_shared<LiteralExpression>(advance());
	} else if (match(TokenType::LEFT_PAREN)) {
		std::shared_ptr<Expression> expr{expression()};
		consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
		return std::make_shared<GroupingExpression>(expr);
	}
	throw parse_error(peek(), "Invalid expression.");
	return nullptr;
}

std::shared_ptr<Expression> Parser::expression() {
	if (check(TokenType::LEFT_BRACE)) {
		return block_expression();
	} else {
		return assignment_expression();
	}
}

std::shared_ptr<BlockExpression> Parser::block_expression() {
	Token opening_block{consume(TokenType::LEFT_BRACE, "Expected left brace.")};
	std::vector<std::shared_ptr<Statement>> stmts{};
	while (!is_at_end()) {
		if (match(TokenType::RIGHT_BRACE)) {
			return std::make_shared<BlockExpression>(stmts, opening_block);
		}

		stmts.push_back(statement());
	}

	throw parse_error(opening_block, "No closing brace found.");
	return nullptr;
}

std::shared_ptr<Expression> Parser::assignment_expression() {
	auto lhs{conditional_expression()};
	while (check(TokenType::EQUAL)) {
		Token op{advance()};
		auto rhs{assignment_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}
	return lhs;
}

std::shared_ptr<Expression> Parser::conditional_expression() {
	return logical_or_expression();
}

std::shared_ptr<Expression> Parser::logical_or_expression() {
	auto lhs{logical_and_expression()};
	while (check(TokenType::OR)) {
		Token op{advance()};
		auto rhs{logical_and_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::logical_and_expression() {
	auto lhs{equality_expression()};
	while (check(TokenType::AND)) {
		Token op{advance()};
		auto rhs{equality_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::equality_expression() {
	auto lhs{relational_expression()};
	while (check({TokenType::EQUAL_EQUAL, TokenType::NOT_EQUAL})) {
		Token op{advance()};
		auto rhs{relational_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::relational_expression() {
	auto lhs{additive_expression()};
	while (check({TokenType::LESS, TokenType::GREATER, TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL})) {
		Token op{advance()};
		auto rhs{additive_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::additive_expression() {
	auto lhs{multiplicative_expression()};
	while (check({TokenType::PLUS, TokenType::MINUS})) {
		Token op{advance()};
		auto rhs{multiplicative_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::multiplicative_expression() {
	auto lhs{cast_expression()};
	while (check({TokenType::STAR, TokenType::SLASH})) {
		Token op{advance()};
		auto rhs{cast_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::cast_expression() {
	auto expr{unary_expression()};
	if (check(TokenType::AS)) {
		expr = std::make_shared<CastExpression>(expr, advance(), type_expression());
	}
	return expr;
}

std::shared_ptr<Expression> Parser::unary_expression() {
	while (check({TokenType::NOT, TokenType::MINUS, TokenType::AMPERSAND, TokenType::STAR})) {
		return prefix_unary_expression();
	}
	return postfix_unary_expression();
}

std::shared_ptr<Expression> Parser::prefix_unary_expression() {
	Token op{advance()};
	auto expr{unary_expression()};
	return std::make_shared<UnaryExpression>(op, expr);
}

std::shared_ptr<Expression> Parser::postfix_unary_expression() {
	auto expr{return_expression()};
	while (match(TokenType::LEFT_PAREN)) {
		auto arg_list{argument_expression_list()};
		expr = std::make_shared<CallExpression>(expr, previous(), arg_list);
	}

	return expr;
}

std::vector<std::shared_ptr<Expression>> Parser::argument_expression_list() {
	std::vector<std::shared_ptr<Expression>> args{};
	if (!check(TokenType::RIGHT_PAREN)) {
		do {
			if (args.size() >= MAX_ARGS) {
				error(peek(), "Cannot have more than 100 arguments.");
			}
			args.push_back(assignment_expression());
		} while (match(TokenType::COMMA));
	}
	
	consume(TokenType::RIGHT_PAREN, "Expected closing parenthesis.");

	return args;
}

std::shared_ptr<Expression> Parser::return_expression() {
	if (check(TokenType::RETURN)) {
		return std::make_shared<ReturnExpression>(advance(), expression());
	}

	return primary_expression();
}

std::shared_ptr<Statement> Parser::statement() {
	if (match(TokenType::SEMICOLON)) { return nullptr; }
	if (match(TokenType::LET)) {
		auto identifier{identifier_expression()};
		if (match(TokenType::LEFT_PAREN)) {
			return function_declaration(identifier);
		} else {
			return variable_declaration(identifier);
		}
	}
	return expression_statement();
}

std::shared_ptr<ExpressionStatement> Parser::expression_statement() {
	auto expr{expression()};
	consume(TokenType::SEMICOLON, "Expected semi-colon after expression statement.");
	return std::make_shared<ExpressionStatement>(expr);
}

std::shared_ptr<VariableDeclarationStatement> Parser::variable_declaration(const std::shared_ptr<IdentifierExpression>& identifier) {
	auto type{std::make_shared<TypeExpression>()};
	if (match(TokenType::COLON)) {
		type = type_expression();
	}

	consume(TokenType::EQUAL, "Expected equal after variable name/type.");

	auto initializer{expression()};
	consume(TokenType::SEMICOLON, "Expected semi-colon after variable declaration statement.");
	return std::make_shared<VariableDeclarationStatement>(
		type,
		identifier,
		initializer
	);
}

std::shared_ptr<FunctionDeclarationStatement> Parser::function_declaration(const std::shared_ptr<IdentifierExpression>& identifier) {
	auto params{parameters()};
	
	auto type{std::make_shared<TypeExpression>()};
	if (match(TokenType::COLON)) {
		type = type_expression();
	}
	
	return std::make_shared<FunctionDeclarationStatement>(
		type,
		identifier,
		params,
		block_expression()
	);
}

std::vector<std::pair<std::shared_ptr<TypeExpression>, std::shared_ptr<IdentifierExpression>>> Parser::parameters() {
	std::vector<std::pair<std::shared_ptr<TypeExpression>, std::shared_ptr<IdentifierExpression>>> params{};
	if (!check(TokenType::RIGHT_PAREN)) {
		do {
			if (params.size() >= MAX_ARGS) {
				error(peek(), "Cannot have more than 100 arguments.");
			}
			std::shared_ptr<TypeExpression> param_type{type_expression()};
			std::shared_ptr<IdentifierExpression> name{identifier_expression()};
			params.push_back(std::make_pair(param_type, name));
		} while (match(TokenType::COMMA));
	}

	consume(TokenType::RIGHT_PAREN, "Expected closing parenthesis.");
	
	return params;
}

std::shared_ptr<TypeExpression> Parser::type_expression() {
	std::shared_ptr<TypeExpression> ret{std::make_shared<TypeExpression>(consume(TokenType::IDENTIFIER, "Expected a type name."))};

	while (match(TokenType::STAR)) {
		ret = std::make_shared<TypeExpression>(TypeExpression{ret});
	}

	return ret;
}

std::shared_ptr<IdentifierExpression> Parser::identifier_expression(const std::string& error_message) {
	return std::make_shared<IdentifierExpression>(consume(TokenType::IDENTIFIER, error_message));
}

