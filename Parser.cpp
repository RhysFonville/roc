#include "Parser.h"
#include "Lexer.h"
#include "Syntax.h"
#include "Types.h"

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

Token Parser::peek() {
	return toks[current];
}

bool Parser::is_at_end() {
	return (peek().type == TokenType::END_OF_FILE);
}

bool Parser::check(TokenType type) {
	if (is_at_end()) return false;
	return (peek().type == type);
}

Token Parser::previous() {
	return toks[current-1];
}

Token Parser::advance() {
	if (!is_at_end()) current++;
	return previous();
}

bool Parser::match(const std::vector<TokenType>& types) {
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

void Parser::synchronize() {
	advance();
	while (!is_at_end()) {
		if (previous().type == TokenType::SEMICOLON) return;
		advance();
	}
}

std::shared_ptr<Expression> Parser::primary_expression() {
	if (match({TokenType::IDENTIFIER})) {
		return std::make_shared<IdentifierExpression>(previous());
	} else if (match(literal_tokens)) {
		return std::make_shared<LiteralExpression>(previous());
	} else if (match({TokenType::LEFT_PAREN})) {
		std::shared_ptr<Expression> expr{expression()};
		consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
		return std::make_shared<GroupingExpression>(expr);
	}
	throw parse_error(peek(), "Invalid expression.");
	return nullptr;
}

std::shared_ptr<Expression> Parser::expression() {
	if (match({TokenType::LEFT_BRACE})) {
		return block_expression();
	} else {
		return assignment_expression();
	}
}

std::shared_ptr<BlockExpression> Parser::block_expression() {
	Token opening_block{previous()};
	std::vector<std::shared_ptr<Statement>> stmts{};
	while (!is_at_end()) {
		if (match({TokenType::RIGHT_BRACE})) {
			return std::make_shared<BlockExpression>(stmts, opening_block);
		}

		stmts.push_back(statement());
	}

	throw parse_error(opening_block, "No closing brace found.");
	return nullptr;
}

std::shared_ptr<Expression> Parser::assignment_expression() {
	auto lhs{conditional_expression()};
	while (match({TokenType::EQUAL})) {
		Token op{previous()};
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
	while (match({TokenType::OR})) {
		Token op{previous()};
		auto rhs{logical_and_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::logical_and_expression() {
	auto lhs{equality_expression()};
	while (match({TokenType::AND})) {
		Token op{previous()};
		auto rhs{equality_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::equality_expression() {
	auto lhs{relational_expression()};
	while (match({TokenType::EQUAL_EQUAL, TokenType::NOT_EQUAL})) {
		Token op{previous()};
		auto rhs{(relational_expression())};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::relational_expression() {
	auto lhs{additive_expression()};
	while (match({TokenType::LESS, TokenType::GREATER, TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL})) {
		Token op{previous()};
		auto rhs{additive_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::additive_expression() {
	auto lhs{multiplicative_expression()};
	while (match({TokenType::PLUS, TokenType::MINUS})) {
		Token op{previous()};
		auto rhs{multiplicative_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::multiplicative_expression() {
	auto lhs{cast_expression()};
	while (match({TokenType::STAR, TokenType::SLASH})) {
		Token op{previous()};
		auto rhs{cast_expression()};
		lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
	}

	return lhs;
}

std::shared_ptr<Expression> Parser::cast_expression() {
	auto expr{unary_expression()};
	if (match({TokenType::AS})) {
		expr = std::make_shared<CastExpression>(expr, previous(), type());
	}
	return expr;
}

std::shared_ptr<Expression> Parser::unary_expression() {
	while (match({TokenType::NOT, TokenType::MINUS, TokenType::AMPERSAND, TokenType::STAR})) {
		return prefix_unary_expression();
	}
	return postfix_unary_expression();
}

std::shared_ptr<Expression> Parser::prefix_unary_expression() {
	Token op{previous()};
	auto expr{unary_expression()};
	return std::make_shared<UnaryExpression>(op, expr);
}

std::shared_ptr<Expression> Parser::postfix_unary_expression() {
	auto expr{return_expression()};
	while (match({TokenType::LEFT_PAREN})) {
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
		} while (match({TokenType::COMMA}));
	}
	
	consume(TokenType::RIGHT_PAREN, "Expected closing parenthesis.");

	return args;
}

std::shared_ptr<Expression> Parser::return_expression() {
	if (match({TokenType::RETURN})) {
		return std::make_shared<ReturnExpression>(previous(), expression());
	}

	return primary_expression();
}

std::shared_ptr<Statement> Parser::statement() {
	if (match({TokenType::SEMICOLON})) { return nullptr; }
	if (match(type_tokens())) {
		return declaration(type(true));
	}
	return expression_statement();
}

std::shared_ptr<ExpressionStatement> Parser::expression_statement() {
	auto expr{expression()};
	consume(TokenType::SEMICOLON, "Expected semi-colon after statement.");
	return std::make_shared<ExpressionStatement>(expr);
}

std::shared_ptr<Statement> Parser::declaration(const Type& type) {
	Token name{consume(TokenType::IDENTIFIER, "Expected identifier.")};
	
	if (match({TokenType::EQUAL})) {
		return variable_declaration(type, name);
	} else if (match({TokenType::LEFT_PAREN})) {
		return function_declaration(type, name);
	} else {
		throw parse_error(advance(), "Unexpected token.");
		return nullptr;
	}
}

std::shared_ptr<VariableDeclarationStatement> Parser::variable_declaration(const Type& type, const Token& name) {
	auto initializer{expression()};
	consume(TokenType::SEMICOLON, "Expected semi-colon after variable declaration statement.");
	return std::make_shared<VariableDeclarationStatement>(
		type,
		std::make_shared<IdentifierExpression>(name),
		initializer
	);
}

std::shared_ptr<FunctionDeclarationStatement> Parser::function_declaration(const Type& type, const Token& name) {
	auto params{parameters()};
	consume(TokenType::LEFT_BRACE, "Expected left brace.");
	return std::make_shared<FunctionDeclarationStatement>(
		type,
		std::make_shared<IdentifierExpression>(name),
		params,
		block_expression()
	);
}

std::vector<std::pair<Type, Token>> Parser::parameters() {
	std::vector<std::pair<Type, Token>> params{};
	if (!check(TokenType::RIGHT_PAREN)) {
		do {
			if (params.size() >= MAX_ARGS) {
				error(peek(), "Cannot have more than 100 arguments.");
			}
			Type param_type{type()};
			Token name{consume(TokenType::IDENTIFIER, "Expected identifier.")};
			params.push_back(std::make_pair(param_type, name));
		} while (match({TokenType::COMMA}));
	}

	consume(TokenType::RIGHT_PAREN, "Expected closing parenthesis.");
	
	return params;
}

Type Parser::type(bool get_previous) {
	Type ret{};
	if (get_previous) {
		if (auto t{token_to_type(previous())}) {
			ret = std::make_shared<TConstructor>(t.value());
		} else {
			ret = nullptr;
		}
	} else {
		if (auto t{token_to_type(consume(type_tokens(), "Expected a type specifier."))})
			ret = std::make_shared<TConstructor>(t.value());
		else
			ret = nullptr;
	}

	while (match({TokenType::STAR})) {
		ret = std::make_shared<TPointer>(ret);
	}

	return ret;
}

