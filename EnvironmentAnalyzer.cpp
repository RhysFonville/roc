#include <memory>
#include "EnvironmentAnalyzer.h"
#include "Lexer.h"
#include "Syntax.h"
#include "Types.h"

void EnvironmentAnalyzer::semantic_error(const Token& token, const std::string& message) {
	error(token, message);
	successful = false;
}

bool EnvironmentAnalyzer::run() {
	for (const std::shared_ptr<Statement>& statement : statements) {
		check_statement(statement);
	}
	return successful;
}

void EnvironmentAnalyzer::check_expression(const std::shared_ptr<Expression>& expr) {
	if (auto id{std::dynamic_pointer_cast<IdentifierExpression>(expr)}) {
		identifier_expression(id);
	} else if (auto lit{std::dynamic_pointer_cast<LiteralExpression>(expr)}) {
		literal_expression(lit);
	} else if (auto group{std::dynamic_pointer_cast<GroupingExpression>(expr)}) {
		grouping_expression(group);
	} else if (auto unary{std::dynamic_pointer_cast<UnaryExpression>(expr)}) {
		unary_expression(unary);
	} else if (auto binary{std::dynamic_pointer_cast<BinaryExpression>(expr)}) {
		binary_expression(binary);
	} else if (auto block{std::dynamic_pointer_cast<BlockExpression>(expr)}) {
		block_expression(block);
	} else if (auto call{std::dynamic_pointer_cast<CallExpression>(expr)}) {
		call_expression(call);
	} else if (auto ret{std::dynamic_pointer_cast<ReturnExpression>(expr)}) {
		return_expression(ret);
	}
}

void EnvironmentAnalyzer::identifier_expression(const std::shared_ptr<IdentifierExpression>& expr) {
	auto type{env_stack.get_identifier_type(expr->identifier)};
	if (!type) {
		semantic_error(expr->identifier, "Identifier not defined.");
		return;
	}
	expr->lvalue = true;
	return;
}

void EnvironmentAnalyzer::literal_expression(const std::shared_ptr<LiteralExpression>& expr) {
	switch (expr->value.type) {
		case TokenType::TRUE:
		case TokenType::FALSE:
			return;
		case TokenType::NUMBER_LITERAL: {
			return;
		}
		case TokenType::STRING_LITERAL:
			return;
		case TokenType::CHAR_LITERAL:
			return;
		default:
			semantic_error(expr->value, "Unknown literal.");
			return;
	}
}

void EnvironmentAnalyzer::grouping_expression(const std::shared_ptr<GroupingExpression>& expr) {
	check_expression(expr->expr);
	expr->lvalue = expr->expr->lvalue;
}

void EnvironmentAnalyzer::unary_expression(const std::shared_ptr<UnaryExpression>& expr) {
	check_expression(expr->expr);

	std::vector<RealType> num_types{
		number_types
		| std::views::values
		| std::ranges::to<std::vector>()
	};

	TConstructor expr_type{con(expr->expr->type).value_or(TConstructor{})};

	switch (expr->op.type) {
		case TokenType::NOT:
			if (is_pointer(expr->expr->type)) {
				semantic_error(expr->op, "Incorrect type. Cannot be a pointer.");
				return;
			}
			if (expr_type != types.at(TypeEnum::BOOL)) {
				semantic_error(expr->op, "Incorrect type. Must be a bool."); 
				return;
			}
			break;
		case TokenType::MINUS:
			if (is_pointer(expr->expr->type)) {
				semantic_error(expr->op, "Incorrect type. Cannot be a pointer.");
				return;
			}
			if (std::ranges::find(num_types, expr_type.type) == num_types.end()) {
				semantic_error(expr->op, "Incorrect type. Must be an number.");
				return;
			}
			break;
		case TokenType::AMPERSAND:
			if (auto lit{std::dynamic_pointer_cast<LiteralExpression>(expr->expr)}) {
				semantic_error(lit->value, "Cannot dereference literal.");
				return;
			}
			if (expr_type == types.at(TypeEnum::NONE)) {
				semantic_error(expr->op, "Incorrect type. Cannot dereference a none type.");
				return;
			}
			break;
		case TokenType::STAR:
			expr->lvalue = true;
			if (!is_pointer(expr->expr->type)) {
				semantic_error(expr->op, "Can only dereference pointer.");
				return;
			}
			break;
		default:
			semantic_error(expr->op, "Unknown expression operator.");
			return;
	}
}

void EnvironmentAnalyzer::binary_expression(const std::shared_ptr<BinaryExpression>& expr) {
	check_expression(expr->sides.first);
	check_expression(expr->sides.second);

	std::vector<RealType> num_types{
		number_types
		| std::views::values
		| std::ranges::to<std::vector>()
	};

	if (con(expr->sides.first->type) != con(expr->sides.second->type)) {
		semantic_error(expr->op, "Mismatched types in binary expression.");
		return;
	}

	TConstructor lhs_type{con(expr->sides.first->type).value_or(TConstructor{})};
	TConstructor rhs_type{con(expr->sides.second->type).value_or(TConstructor{})};

	switch (expr->op.type) {
		case TokenType::PLUS:
		case TokenType::MINUS:
		case TokenType::STAR:
		case TokenType::SLASH:
			if (std::ranges::find(num_types, lhs_type.type) == num_types.end() ||
				std::ranges::find(num_types, rhs_type.type) == num_types.end()) {
				semantic_error(expr->op, "Incorrect type. Must be an number.");
				return;
			}
			return;
		case TokenType::GREATER:
		case TokenType::GREATER_EQUAL:
		case TokenType::LESS:
		case TokenType::LESS_EQUAL:
			if (std::ranges::find(num_types, lhs_type.type) != num_types.end() ||
				std::ranges::find(num_types, rhs_type.type) != num_types.end()) {
				semantic_error(expr->op, "Incorrect type. Must be a bool or number.");
				return;
			}
			return;
		case TokenType::NOT_EQUAL:
		case TokenType::EQUAL_EQUAL:
			if (std::ranges::find(num_types, lhs_type.type) != num_types.end() ||
				lhs_type != types.at(TypeEnum::BOOL)) {
				semantic_error(expr->op, "Incorrect type. Must be a bool or number.");
				return;
			}
			return;
		case TokenType::AND:
		case TokenType::OR:
			if (lhs_type != types.at(TypeEnum::BOOL) ||
				rhs_type != types.at(TypeEnum::BOOL)) {
				semantic_error(expr->op, "Incorrect type. Must be a bool.");
				return;
			}
			return;
		case TokenType::EQUAL: {
			if (!expr->sides.first->lvalue) {
				semantic_error(expr->op, "LHS must be a lvalue.");
				return;
			}

			if (lhs_type != rhs_type) {
				semantic_error(expr->op, "Incorrect type. RHS must equal LHS.");
				return;
			}

			expr->lvalue = true;

			return;
		}
		default:
			semantic_error(expr->op, "Unknown binary operator.");
			return;
	}
}

void EnvironmentAnalyzer::block_expression(const std::shared_ptr<BlockExpression>& expr, const std::vector<Variable>& vars) {
	env_stack.push(Environment{});

	for (const auto& var : vars) {
		env_stack.back().variables.insert(var);
	}

	for (auto stmt : expr->statements) {
		check_statement(stmt);
	}

	env_stack.pop();
}

void EnvironmentAnalyzer::call_expression(const std::shared_ptr<CallExpression>& expr) {
	check_expression(expr->callee);

	Function func{};
	if (auto id{std::dynamic_pointer_cast<IdentifierExpression>(expr->callee)}) {
		if (auto opt_func{env_stack.get_function(id->identifier)}; !opt_func.has_value()) {
			semantic_error(id->identifier, "No function of that name.");
		} else {
			func = opt_func.value();
		}
	} else {
		semantic_error(expr->closing_paren, "Function callee must be an identifier");
	}

	if (expr->args.size() != func.args.size()) {
		semantic_error(expr->closing_paren, "Different number of arguments than parameters.");
	}

	for (int i = 0; i < std::min({expr->args.size(), func.args.size()}); i++) {
		check_expression(expr->args[i]);

		if (!comp_types(expr->args[i]->type, func.args[i].type)) {
			semantic_error(expr->closing_paren, "Mismatched types between argument and parameter.");
	  	}
	}
}

void EnvironmentAnalyzer::return_expression(const std::shared_ptr<ReturnExpression>& expr) {
	check_expression(expr->return_expression);
}

void EnvironmentAnalyzer::check_statement(const std::shared_ptr<Statement>& statement) {
	if (auto expr{std::dynamic_pointer_cast<ExpressionStatement>(statement)}) {
		expression_statement(expr);
	} else if (auto decl{std::dynamic_pointer_cast<VariableDeclarationStatement>(statement)}) {
		variable_declaration_statement(decl);
	} else if (auto decl{std::dynamic_pointer_cast<FunctionDeclarationStatement>(statement)}) {
		function_declaration_statement(decl);
	}
}

void EnvironmentAnalyzer::expression_statement(const std::shared_ptr<ExpressionStatement>& stmt) {
	check_expression(stmt->expr);
	return;
}

void EnvironmentAnalyzer::variable_declaration_statement(const std::shared_ptr<VariableDeclarationStatement>& stmt) {
	if (env_stack.has_identifier(stmt->identifier->identifier)) {
		semantic_error(stmt->identifier->identifier, "Identifier already defined.");
		return;
	}
	check_expression(stmt->initializer);

	if (comp_types(stmt->type, std::make_shared<TConstructor>(types.at(TypeEnum::NONE)))) {
		semantic_error(stmt->identifier->identifier, "Cannot declare variable of type none.");
		return;
	}
	if (!comp_types(stmt->initializer->type, stmt->type)) {
		semantic_error(stmt->identifier->identifier, "Incorrect type.");
		return;
	}
	env_stack.back().variables.insert(Variable{stmt->type, stmt->identifier->identifier});
}

void EnvironmentAnalyzer::function_declaration_statement(const std::shared_ptr<FunctionDeclarationStatement>& stmt) {
	if (env_stack.has_identifier(stmt->identifier->identifier)) {
		semantic_error(stmt->identifier->identifier, "Identifier already defined.");
		return;
	}

	std::vector<Variable> params{(stmt->params | std::views::transform([](const std::pair<Type, Token>& param) {
		return Variable{param.first, param.second};
	})) | std::ranges::to<std::vector>()};

	EnvironmentStack env_stack_copy{env_stack};
	env_stack.envs.erase(env_stack.envs.begin()+1, env_stack.envs.end());

	block_expression(stmt->block, params);

	env_stack = env_stack_copy;

	if (!comp_types(stmt->block->type, stmt->return_type)) {
		semantic_error(stmt->identifier->identifier, "Block is not the same type as specified function return type.");
	}

	env_stack.back().functions.insert(Function{stmt->return_type, stmt->identifier->identifier, params});
}

