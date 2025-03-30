#pragma once

#include <exception>
#include "Syntax.h"
#include "ErrorHandling.h"

class ParserException : public std::runtime_error {
public:
	ParserException(const Token& tok, const std::string& what)
		: runtime_error{what}, tok{tok} { }

	Token get_tok() const noexcept { return tok; }
private:
	Token tok{};
};

class Parser {
public:
	Parser(const std::vector<Token>& toks) : toks{toks} { }

	std::vector<std::shared_ptr<Statement>> run();

private:
	std::vector<Token> toks;
	int current{};

	static constexpr uint8_t MAX_ARGS{100};

	ParserException parse_error(const Token& tok, const std::string& message) const noexcept;

	Token peek();
	bool is_at_end();
	bool check(TokenType type);
	Token previous();
	Token advance();
	bool match(const std::vector<TokenType>& types);
	Token consume(TokenType type, const std::string& message);
	Token consume(const std::vector<TokenType>& types, const std::string& message);

	void synchronize();
	
	std::shared_ptr<Expression> primary_expression();
	std::shared_ptr<Expression> expression();
	std::shared_ptr<BlockExpression> block_expression();
	std::shared_ptr<Expression> assignment_expression();
	std::shared_ptr<Expression> conditional_expression();
	std::shared_ptr<Expression> logical_or_expression();
	std::shared_ptr<Expression> logical_and_expression();
	std::shared_ptr<Expression> equality_expression();
	std::shared_ptr<Expression> relational_expression();
	std::shared_ptr<Expression> additive_expression();
	std::shared_ptr<Expression> multiplicative_expression();
	std::shared_ptr<Expression> unary_expression();
	std::shared_ptr<Expression> prefix_unary_expression();
	std::shared_ptr<Expression> postfix_unary_expression();
	std::vector<std::shared_ptr<Expression>> argument_expression_list();
	std::shared_ptr<Expression> return_expression();
	std::shared_ptr<Statement> statement();
	std::shared_ptr<ExpressionStatement> expression_statement();
	std::shared_ptr<Statement> declaration(const Type& type);
	std::shared_ptr<VariableDeclarationStatement> variable_declaration(const Type& type, const Token& name);
	std::shared_ptr<FunctionDeclarationStatement> function_declaration(const Type& type, const Token& name);
	std::vector<std::pair<Type, Token>> parameters();
	Type type(bool get_previous = false);
};

