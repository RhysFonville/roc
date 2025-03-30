#pragma once

#include <utility>
#include <memory>
#include <variant>
#include <iostream>
#include "Types.h"

struct Expression {
	virtual ~Expression() = default;

	virtual void print() { type->print(); }

	Type type{};
	bool lvalue{};
};

struct UnaryExpression : public Expression {
	explicit UnaryExpression(const Token& op, std::shared_ptr<Expression>& expr)
		: op{op}, expr{expr} { }
	
	Token op{};
	std::shared_ptr<Expression> expr{};
};

struct BinaryExpression : public Expression {
	explicit BinaryExpression(std::shared_ptr<Expression>& lhs, const Token& op, std::shared_ptr<Expression>& rhs)
		: sides{std::make_pair(lhs, rhs)}, op{op} { }

	std::pair<std::shared_ptr<Expression>, std::shared_ptr<Expression>> sides{};
	Token op{};
};

struct GroupingExpression : public Expression {
	explicit GroupingExpression(std::shared_ptr<Expression>& expr)
		: expr{std::move(expr)} { }

	std::shared_ptr<Expression> expr{};
};

struct LiteralExpression : public Expression {
	explicit LiteralExpression(const Token& value)
		: value{value} { }

	Token value{};
};

struct IdentifierExpression : public Expression {
	explicit IdentifierExpression(const Token& identifier)
		: identifier{identifier} { }
	
	Token identifier{};
};

struct CallExpression : public Expression {
	explicit CallExpression(const std::shared_ptr<Expression>& callee,
		const Token& closing_paren, const std::vector<std::shared_ptr<Expression>>& args)
		: callee{callee}, closing_paren{closing_paren}, args{args} { }

	std::shared_ptr<Expression> callee{};
	Token closing_paren{};
	std::vector<std::shared_ptr<Expression>> args{};
};

struct ReturnExpression : public Expression {
	explicit ReturnExpression (const Token& return_token, const std::shared_ptr<Expression>& return_expression)
	: return_tok{return_token}, return_expression{return_expression} { }

	Token return_tok{};
	std::shared_ptr<Expression> return_expression{};
};

struct Statement {
	virtual ~Statement() = default;
};

struct BlockExpression : public Expression {
	explicit BlockExpression(const std::vector<std::shared_ptr<Statement>>& statements,
		const Token& opening_block) : statements{statements}, opening_block{opening_block} { }
	
	std::vector<std::shared_ptr<Statement>> statements{};
	Token opening_block{};
};

struct ExpressionStatement : public Statement {
	explicit ExpressionStatement(const std::shared_ptr<Expression>& expr)
		: expr{expr} { }
	
	std::shared_ptr<Expression> expr{};
};

struct VariableDeclarationStatement : public Statement {
	explicit VariableDeclarationStatement(const Type& type, const std::shared_ptr<IdentifierExpression>& identifier, const std::shared_ptr<Expression>& initializer)
		: type{type}, identifier{identifier}, initializer{initializer} { }

	Type type{};
	std::shared_ptr<IdentifierExpression> identifier{};
	std::shared_ptr<Expression> initializer{};
};

struct FunctionDeclarationStatement : public Statement {
	explicit FunctionDeclarationStatement(const Type& return_type,
		const std::shared_ptr<IdentifierExpression>& identifier,
		const std::vector<std::pair<Type, Token>>& params,
		const std::shared_ptr<BlockExpression>& block)
		: return_type{return_type}, identifier{identifier}, params{params}, block{block} { }

	Type return_type{};
	std::shared_ptr<IdentifierExpression> identifier{};
	std::vector<std::pair<Type, Token>> params{};
	std::shared_ptr<BlockExpression> block{};
};

