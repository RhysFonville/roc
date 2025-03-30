#pragma once

#include <memory>
#include <optional>
#include <set>
#include <ranges>
#include <algorithm>
#include <limits>
#include "Lexer.h"
#include "Syntax.h"
#include "ErrorHandling.h"
#include "Environment.h"
#include "Types.h"

class EnvironmentAnalyzer {
public:
	EnvironmentAnalyzer (const std::vector<std::shared_ptr<Statement>>& statements)
		: statements{statements} { }

	bool run();

private:
	std::vector<std::shared_ptr<Statement>> statements{};

	bool successful{true};

	static std::optional<TConstructor> con(const Type& type) noexcept {
		if (auto c{std::dynamic_pointer_cast<TConstructor>(type)}) {
			return *c;
		} else {
			return std::nullopt;
		}
	}

	void semantic_error(const Token& token, const std::string& message);

	void check_expression(const std::shared_ptr<Expression>& expr);
	void identifier_expression(const std::shared_ptr<IdentifierExpression>& expr);
	void literal_expression(const std::shared_ptr<LiteralExpression>& expr);
	void grouping_expression(const std::shared_ptr<GroupingExpression>& expr);
	void unary_expression(const std::shared_ptr<UnaryExpression>& expr);
	void binary_expression(const std::shared_ptr<BinaryExpression>& expr);
	void block_expression(const std::shared_ptr<BlockExpression>& expr, const std::vector<Variable>& vars = {});
	void call_expression(const std::shared_ptr<CallExpression>& expr);
	void return_expression(const std::shared_ptr<ReturnExpression>& expr);
	
	void check_statement(const std::shared_ptr<Statement>& statement);
	void expression_statement(const std::shared_ptr<ExpressionStatement>& stmt);
	void variable_declaration_statement(const std::shared_ptr<VariableDeclarationStatement>& stmt);
	void function_declaration_statement(const std::shared_ptr<FunctionDeclarationStatement>& stmt);

	EnvironmentStack env_stack{};
};
