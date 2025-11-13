#pragma once

#include <utility>
#include <memory>
#include <ostream>
#include "Types.h"

struct Expression {
	virtual ~Expression() = default;

	virtual void print(std::ostream& os = std::cout) const noexcept {

    }

	virtual void println(std::ostream& os = std::cout) const noexcept {
		print(os);
		os << std::endl;
	}

	MixType type{};
	bool lvalue{};
};

struct UnaryExpression : public Expression {
	explicit UnaryExpression(const Token& op, std::shared_ptr<Expression>& expr)
		: op{op}, expr{expr} { }

	void print(std::ostream& os = std::cout) const noexcept {
		//Expression::println(os);
		os << op << ' ';
		expr->print(os);
	}
	
	Token op{};
	std::shared_ptr<Expression> expr{};
};

struct BinaryExpression : public Expression {
	explicit BinaryExpression(std::shared_ptr<Expression>& lhs, const Token& op, std::shared_ptr<Expression>& rhs)
		: sides{std::make_pair(lhs, rhs)}, op{op} { }

	void print(std::ostream& os = std::cout) const noexcept {
		//Expression::println(os);
		sides.first->println(os);
		os << op << std::endl;
		sides.second->print(os);
	}

	std::pair<std::shared_ptr<Expression>, std::shared_ptr<Expression>> sides{};
	Token op{};
};

struct GroupingExpression : public Expression {
	explicit GroupingExpression(std::shared_ptr<Expression>& expr)
		: expr{std::move(expr)} { }

	void print(std::ostream& os = std::cout) const noexcept {
		//Expression::println(os);
		os << "(\n";
		expr->println(os);
		os << ")";
	}

	std::shared_ptr<Expression> expr{};
};

struct LiteralExpression : public Expression {
	explicit LiteralExpression(const Token& value)
		: value{value} { }

	void print(std::ostream& os = std::cout) const noexcept override {
		//Expression::println(os);
		os << value;
	}

	Token value{};
};

struct IdentifierExpression : public Expression {
	explicit IdentifierExpression(const Token& identifier)
		: identifier{identifier} { }

	void print(std::ostream& os = std::cout) const noexcept override {
		//Expression::println(os);
		os << identifier;
	}
	
	Token identifier{};
};

struct CallExpression : public Expression {
	explicit CallExpression(const std::shared_ptr<Expression>& callee,
		const Token& closing_paren, const std::vector<std::shared_ptr<Expression>>& args)
		: callee{callee}, closing_paren{closing_paren}, args{args} { }

	void print(std::ostream& os = std::cout) const noexcept override {
		//Expression::println(os);
		callee->print(os);
		os << "(args:\n";
		for (const auto& arg : args) {
			arg->println(os);
			os << ",\n";
		}
		os << closing_paren;
	}

	std::shared_ptr<Expression> callee{};
	Token closing_paren{};
	std::vector<std::shared_ptr<Expression>> args{};
};

struct ReturnExpression : public Expression {
	explicit ReturnExpression (const Token& return_token, const std::shared_ptr<Expression>& return_expression)
	: return_tok{return_token}, return_expression{return_expression} { }

	void print(std::ostream& os = std::cout) const noexcept override {
		os << return_tok << std::endl;
		return_expression->print(os);
	}

	Token return_tok{};
	std::shared_ptr<Expression> return_expression{};
};

struct TypeExpression : public Expression {
	explicit TypeExpression()
		: parse_type{ParseType{}} { }
	explicit TypeExpression(const Token& name)
		: parse_type{ParseType{name}} { }
	explicit TypeExpression(const std::shared_ptr<TypeExpression>& ptr)
		: parse_type{ParseType{ptr->parse_type.get_parse_type()}} { }

	void print(std::ostream& os = std::cout) const noexcept override {
		parse_type.print(os);
	}

	MixType parse_type;
};

struct CastExpression : public Expression {
	explicit CastExpression(const std::shared_ptr<Expression>& expr,
		const Token& as, const std::shared_ptr<TypeExpression>& cast_type)
		: expr{expr}, as{as}, cast_type{cast_type} { }
	
	void print(std::ostream& os = std::cout) const noexcept override {
		//Expression::println(os);
		expr->println(os);
		os << as << std::endl;
		cast_type->print(os);
	}

	std::shared_ptr<Expression> expr{};
	Token as{};
	std::shared_ptr<TypeExpression> cast_type{};
};

struct Statement {
	virtual ~Statement() = default;

	virtual void print(std::ostream& os = std::cout) const noexcept = 0;

	void println(std::ostream& os = std::cout) const noexcept {
		print(os);
		os << std::endl;
	}
};

struct BlockExpression : public Expression {
	explicit BlockExpression(const std::vector<std::shared_ptr<Statement>>& statements,
		const Token& opening_block) : statements{statements}, opening_block{opening_block} { }

	void print(std::ostream& os = std::cout) const noexcept {
		os << opening_block << std::endl;
		for (const auto& stmt : statements) {
			stmt->println(os);
		}
		os << '}';
	}
	
	std::vector<std::shared_ptr<Statement>> statements{};
	Token opening_block{};
};

struct ExpressionStatement : public Statement {
	explicit ExpressionStatement(const std::shared_ptr<Expression>& expr)
		: expr{expr} { }
	
	void print(std::ostream& os = std::cout) const noexcept override {
		os << expr;
	}

	std::shared_ptr<Expression> expr{};
};

struct VariableDeclarationStatement : public Statement {
	explicit VariableDeclarationStatement(const std::shared_ptr<TypeExpression>& type, const std::shared_ptr<IdentifierExpression>& identifier, const std::shared_ptr<Expression>& initializer)
		: type{type}, identifier{identifier}, initializer{initializer} { }

	void print(std::ostream& os = std::cout) const noexcept override {
		type->print(os);
		os << ' ';
		identifier->print(os);
		os << "\n";
		initializer->print(os);
	}

	std::shared_ptr<TypeExpression> type{};
	std::shared_ptr<IdentifierExpression> identifier{};
	std::shared_ptr<Expression> initializer{};
};

struct FunctionDeclarationStatement : public Statement {
	explicit FunctionDeclarationStatement(const std::shared_ptr<TypeExpression>& return_type,
		const std::shared_ptr<IdentifierExpression>& identifier,
		const std::vector<std::pair<std::shared_ptr<TypeExpression>, std::shared_ptr<IdentifierExpression>>>& params,
		const std::shared_ptr<BlockExpression>& block)
		: return_type{return_type}, identifier{identifier}, params{params}, block{block} { }
	
	void print(std::ostream& os = std::cout) const noexcept override {
		return_type->print(os);
		os << ' ';
		identifier->print(os);
		os << "( ";
		for (const auto& p : params) {
			p.first->print(os);
			os << ':' << p.second;
			os << ' ';
		}
		os << ")\n";

		block->print(os);
	}

	std::shared_ptr<TypeExpression> return_type{};
	std::shared_ptr<IdentifierExpression> identifier{};
	std::vector<std::pair<std::shared_ptr<TypeExpression>, std::shared_ptr<IdentifierExpression>>> params{};
	std::shared_ptr<BlockExpression> block{};
};

