#include <memory>
#include <algorithm>
#include <assert.h>
#include "TypeAnalyzer.h"
#include "ErrorHandling.h"
#include "Lexer.h"
#include "Syntax.h"
#include "Types.h"

TypeAnalyzer::TypeAnalyzer(const std::vector<std::shared_ptr<Statement>>& stmts) : stmts{stmts} { }

void TypeAnalyzer::type_error(const Token& token, const std::string& message) {
	error(token, message);
	success = false;
}

void TypeAnalyzer::unify(const Type& t1, const Type& t2) {
	if (auto var1 = std::dynamic_pointer_cast<TVariable>(t1)) {
		if (auto var2 = std::dynamic_pointer_cast<TVariable>(t2); var2 && var1->index == var2->index) {
			return;
		}
		if (var1->index < substitution.size() && substitution[var1->index] != nullptr && substitution[var1->index] != t1) {
			unify(substitution[var1->index], t2);
			return;
		}
		if (occurs_in(var1->index, t2)) {
			//throw std::runtime_error("Infinite type: $" + std::to_string(var1->index) + " = " /*+ substitute(t2)*/);
			return;
		}
		if (var1->index >= substitution.size()) {
			substitution.resize(var1->index + 1);
		}
		substitution[var1->index] = t2;
		return;
	}
	
	if (auto var2 = std::dynamic_pointer_cast<TVariable>(t2)) {
		if (var2->index < substitution.size() && substitution[var2->index] != nullptr && substitution[var2->index] != t2) {
			unify(t1, substitution[var2->index]);
			return;
		}
		if (occurs_in(var2->index, t1)) {
			//throw std::runtime_error("Infinite type: $" + std::to_string(var2->index) + " = " /*+ substitute(t1)*/);
			return;
		}
		if (var2->index >= substitution.size()) {
			substitution.resize(var2->index + 1);
		}
		substitution[var2->index] = t1;
		return;
	}

	auto cons1 = std::dynamic_pointer_cast<TConstructor>(t1);
	auto cons2 = std::dynamic_pointer_cast<TConstructor>(t2);
	if (cons1 && cons2) {
		if (cons1->type != cons2->type || cons1->generics.size() != cons2->generics.size()) {
			//throw std::runtime_error("Type mismatch: " /*+ substitute(t1) + " vs. " + substitute(t2)*/);
			return;
		}
		for (size_t i = 0; i < cons1->generics.size(); ++i) {
			unify(cons1->generics[i], cons2->generics[i]);
		}
		return;
	}

	if (auto p1 = std::dynamic_pointer_cast<TPointer>(t1)) {
        if (auto p2 = std::dynamic_pointer_cast<TPointer>(t2)) {
            unify(p1->inner, p2->inner);
            return;
        }
    }
	
	//throw std::runtime_error("Unification failed");
}

bool TypeAnalyzer::occurs_in(int index, const Type& t) {
	if (auto var = std::dynamic_pointer_cast<TVariable>(t)) {
		if (var->index < substitution.size() && substitution[var->index] != nullptr && substitution[var->index] != t) {
			return occurs_in(index, substitution[var->index]);
		}
		return var->index == index;
	} else if (auto cons = std::dynamic_pointer_cast<TConstructor>(t)) {
		return std::ranges::any_of(cons->generics, [&](const Type& sub) { return occurs_in(index, sub); });
	} else if (auto ptr = std::dynamic_pointer_cast<TPointer>(t)) {
        return occurs_in(index, ptr->inner);
    }
	return false;
}

Type TypeAnalyzer::substitute(const Type& t) {
	if (auto var = std::dynamic_pointer_cast<TVariable>(t)) {
		if (var->index < substitution.size() && substitution[var->index] != nullptr && substitution[var->index] != t) {
			return substitute(substitution[var->index]);
		}
	} else if (auto cons = std::dynamic_pointer_cast<TConstructor>(t)) {
		std::vector<Type> newGenerics;
		newGenerics.reserve(cons->generics.size());
		for (const auto& sub : cons->generics) {
			newGenerics.push_back(substitute(sub));
		}
		return std::make_shared<TConstructor>(TConstructor{cons->type, std::move(newGenerics)});
	} else if (auto ptr = std::dynamic_pointer_cast<TPointer>(t)) {
        return std::make_shared<TPointer>(substitute(ptr->inner));
    }
	return t;
}

void TypeAnalyzer::solve_constraints() {
	for (auto constraint : type_constraints) {
		if (auto eq{std::dynamic_pointer_cast<CEquality>(constraint)}) {
			unify(eq->t1, eq->t2);
		}
	}
	type_constraints.clear();
}

bool TypeAnalyzer::run() {
	for (auto stmt : stmts) {
		infer_statement(stmt);
	}
	solve_constraints();
	for (auto stmt : stmts) {
		substitute_statement(stmt);
	}

	return success;
}

void TypeAnalyzer::infer_expression(const std::shared_ptr<Expression>& expr) {
	if (auto id{std::dynamic_pointer_cast<IdentifierExpression>(expr)}) {
		infer_identifier_expression(id);
	} else if (auto lit{std::dynamic_pointer_cast<LiteralExpression>(expr)}) {
		infer_literal_expression(lit);
	} else if (auto group{std::dynamic_pointer_cast<GroupingExpression>(expr)}) {
		infer_grouping_expression(group);
	} else if (auto un{std::dynamic_pointer_cast<UnaryExpression>(expr)}) {
		infer_unary_expression(un);
	} else if (auto bin{std::dynamic_pointer_cast<BinaryExpression>(expr)}) {
		infer_binary_expression(bin);
	} else if (auto block{std::dynamic_pointer_cast<BlockExpression>(expr)}) {
		infer_block_expression(block);
	} else if (auto call{std::dynamic_pointer_cast<CallExpression>(expr)}) {
		infer_call_expression(call);
	} else if (auto ret{std::dynamic_pointer_cast<ReturnExpression>(expr)}) {
		infer_return_expression(ret);
	} else if (auto cast{std::dynamic_pointer_cast<CastExpression>(expr)}) {
		infer_cast_expression(cast);
	} else {
		expr->type = nullptr;
	}
}

void TypeAnalyzer::infer_identifier_expression(const std::shared_ptr<IdentifierExpression>& expr) {
	if (auto t{env_stack.get_identifier_type(expr->identifier)}) {
		expr->type = t.value();
	} else {
		type_error(expr->identifier, "Identifier not defined.");
	}
}

void TypeAnalyzer::infer_literal_expression(const std::shared_ptr<LiteralExpression>& expr) {
	switch (expr->value.type) {
		case TokenType::TRUE:
		case TokenType::FALSE:
			expr->type = std::make_shared<TConstructor>(types.at(TypeEnum::BOOL));
			return;
		case TokenType::NUMBER_LITERAL:
			for (const auto& type : number_types | std::views::values) {
				if (expr->value.value.ends_with(type.keyword.first)) {
					expr->type = std::make_shared<TConstructor>(type);
					return;
				}
			}
			expr->type = fresh_type_variable();
			return;
		case TokenType::STRING_LITERAL:
			expr->type = std::make_shared<TPointer>(std::make_shared<TConstructor>(types.at(TypeEnum::I8)));
			return;
		case TokenType::CHAR_LITERAL:
			expr->type = std::make_shared<TConstructor>(types.at(TypeEnum::I8));
			return;
		default:
			expr->type = nullptr;
			return;
	}
}

void TypeAnalyzer::infer_grouping_expression(const std::shared_ptr<GroupingExpression>& expr) {
	infer_expression(expr->expr);
	expr->type = expr->expr->type;
}

void TypeAnalyzer::infer_unary_expression(const std::shared_ptr<UnaryExpression>& expr) {
	infer_expression(expr->expr);
	switch (expr->op.type) {
		case TokenType::NOT:
			expr->type = std::make_shared<TConstructor>(types.at(TypeEnum::BOOL));
			return;
		case TokenType::MINUS:
			expr->type = expr->expr->type;
			return;
		case TokenType::STAR:
			if (is_pointer(expr->expr->type)) {
				expr->type = std::dynamic_pointer_cast<TPointer>(expr->expr->type)->inner;
			} else {
				expr->type = fresh_type_variable();
				type_constraints.push_back(std::make_shared<CEquality>(expr->expr->type, std::make_shared<TPointer>(expr->type)));
			}
			return;
		case TokenType::AMPERSAND:
			expr->type = std::make_shared<TPointer>(expr->expr->type);
			return;
		default:
			type_error(expr->op, "Invalid unary operation.");
	}
}

void TypeAnalyzer::infer_binary_expression(const std::shared_ptr<BinaryExpression>& expr) {
	infer_expression(expr->sides.first);
	infer_expression(expr->sides.second);

	type_constraints.push_back(std::make_shared<CEquality>(expr->sides.first->type, expr->sides.second->type));

	switch (expr->op.type) {
		case TokenType::PLUS:
		case TokenType::MINUS:
		case TokenType::STAR:
		case TokenType::SLASH:
		case TokenType::EQUAL_EQUAL:
		case TokenType::NOT_EQUAL:
		case TokenType::GREATER:
		case TokenType::GREATER_EQUAL:
		case TokenType::LESS:
		case TokenType::LESS_EQUAL:
		case TokenType::EQUAL:
			expr->type = fresh_type_variable();
			type_constraints.push_back(std::make_shared<CEquality>(expr->type, expr->sides.first->type));
			break;
		case TokenType::AND:
		case TokenType::OR:
			expr->type = std::make_shared<TConstructor>(types.at(TypeEnum::BOOL));
			break;
		default:
			type_error(expr->op, "Invalid binary operation.");
	}
}

void TypeAnalyzer::infer_block_expression(const std::shared_ptr<BlockExpression>& expr, const std::shared_ptr<FunctionDeclarationStatement>& func) {
	env_stack.push(Environment{});

	if (func != nullptr) {
		for (const auto& param : func->params) {
			env_stack.back().variables.insert(Variable{param.first, param.second});
		}
	}

	expr->type = fresh_type_variable();

	std::vector<ReturnExpression> rets{};
	for (auto& stmt : expr->statements) {
		infer_statement(stmt);

		if (auto expr_stmt{std::dynamic_pointer_cast<ExpressionStatement>(stmt)}) {
			if (auto ret{std::dynamic_pointer_cast<ReturnExpression>(expr_stmt->expr)}) {
				rets.push_back(*ret);
			}
		}
	}

	if (rets.empty()) {
		expr->type = std::make_shared<TConstructor>(types.at(TypeEnum::NONE));
	} else {
		for (auto& ret : rets) {
			type_constraints.push_back(std::make_shared<CEquality>(expr->type, rets[0].type));
		}
	}

	env_stack.pop();
}

void TypeAnalyzer::infer_call_expression(const std::shared_ptr<CallExpression>& expr) {
	Function func{env_stack.get_function(std::static_pointer_cast<IdentifierExpression>(expr->callee)->identifier).value()};
	expr->type = func.return_type;

	infer_expression(expr->callee);

	if (func.args.size() != expr->args.size()) {
		type_error(expr->closing_paren, "Different number of arguments than parameters.");
	}

	for (int i{0}; i < std::min(func.args.size(), expr->args.size()); i++) {
		infer_expression(expr->args[i]);
		type_constraints.push_back(std::make_shared<CEquality>(expr->args[i]->type, func.args[i].type));
	}
}

void TypeAnalyzer::infer_return_expression(const std::shared_ptr<ReturnExpression>& expr) {
	infer_expression(expr->return_expression);
	expr->type = expr->return_expression->type;
}

void TypeAnalyzer::infer_cast_expression(const std::shared_ptr<CastExpression>& expr) {
	infer_expression(expr->expr);
	expr->type = expr->cast_type;
}

void TypeAnalyzer::infer_statement(const std::shared_ptr<Statement>& stmt) {
	if (auto expr{std::dynamic_pointer_cast<ExpressionStatement>(stmt)}) {
		infer_expression_statement(expr);
	} else if (auto var_decl{std::dynamic_pointer_cast<VariableDeclarationStatement>(stmt)}) {
		infer_variable_declaration_statement(var_decl);
	} else if (auto func_decl{std::dynamic_pointer_cast<FunctionDeclarationStatement>(stmt)}) {
		infer_function_declaration_statement(func_decl);
	}
}

void TypeAnalyzer::infer_expression_statement(const std::shared_ptr<ExpressionStatement>& stmt) {
	infer_expression(stmt->expr);
}

void TypeAnalyzer::infer_variable_declaration_statement(const std::shared_ptr<VariableDeclarationStatement>& stmt) {
	if (stmt->type == nullptr) {
		stmt->type = fresh_type_variable();
	}

	infer_expression(stmt->initializer);

	type_constraints.push_back(std::make_shared<CEquality>(stmt->type, stmt->initializer->type));

	env_stack.back().variables.insert(Variable{stmt->type, stmt->identifier->identifier});
}

void TypeAnalyzer::infer_function_declaration_statement(const std::shared_ptr<FunctionDeclarationStatement>& stmt) {
	if (stmt->return_type == nullptr) {
		stmt->return_type = fresh_type_variable();
	}

	for (auto& param : stmt->params) {
		if (param.first == nullptr) {
			param.first = fresh_type_variable();
		}
	}

	EnvironmentStack env_stack_copy{env_stack};
	env_stack.envs.erase(env_stack.envs.begin()+1, env_stack.envs.end());

	infer_block_expression(stmt->block, stmt);

	env_stack = env_stack_copy;

	type_constraints.push_back(std::make_shared<CEquality>(stmt->return_type, stmt->block->type));

	std::vector<Variable> params{(stmt->params | std::views::transform([](const std::pair<Type, Token>& param) {
		return Variable{param.first, param.second};
	})) | std::ranges::to<std::vector>()};

	env_stack.back().functions.insert(Function{stmt->return_type, stmt->identifier->identifier, params});
}

void TypeAnalyzer::substitute_expression(const std::shared_ptr<Expression>& expr) {
	if (auto id{std::dynamic_pointer_cast<IdentifierExpression>(expr)}) {
		substitute_identifier_expression(id);
	} else if (auto lit{std::dynamic_pointer_cast<LiteralExpression>(expr)}) {
		substitute_literal_expression(lit);
	} else if (auto group{std::dynamic_pointer_cast<GroupingExpression>(expr)}) {
		substitute_grouping_expression(group);
	} else if (auto un{std::dynamic_pointer_cast<UnaryExpression>(expr)}) {
		substitute_unary_expression(un);
	} else if (auto bin{std::dynamic_pointer_cast<BinaryExpression>(expr)}) {
		substitute_binary_expression(bin);
	} else if (auto block{std::dynamic_pointer_cast<BlockExpression>(expr)}) {
		substitute_block_expression(block);
	} else if (auto call{std::dynamic_pointer_cast<CallExpression>(expr)}) {
		substitute_call_expression(call);
	} else if (auto ret{std::dynamic_pointer_cast<ReturnExpression>(expr)}) {
		substitute_return_expression(ret);
	} else if (auto cast{std::dynamic_pointer_cast<CastExpression>(expr)}) {
		substitute_cast_expression(cast);
	}
}

void TypeAnalyzer::substitute_identifier_expression(const std::shared_ptr<IdentifierExpression>& expr) {
	expr->type = substitute(expr->type);

	if (!is_inferred(expr->type)) {
		type_error(expr->identifier, "Unable to infer identifier type.");
	}
}

void TypeAnalyzer::substitute_literal_expression(const std::shared_ptr<LiteralExpression>& expr) {
	expr->type = substitute(expr->type);

	/*if (expr->value.type == TokenType::NUMBER_LITERAL && std::dynamic_pointer_cast<TVariable>(expr->type)) {
		expr->type = std::make_shared<TConstructor>(types.at(TypeEnum::I32));
	}*/

	if (!is_inferred(expr->type)) {
		type_error(expr->value, "Unable to infer literal type.");
	}
}

void TypeAnalyzer::substitute_grouping_expression(const std::shared_ptr<GroupingExpression>& expr) {
	expr->type = substitute(expr->type);
}

void TypeAnalyzer::substitute_unary_expression(const std::shared_ptr<UnaryExpression>& expr) {
	substitute_expression(expr->expr);
	expr->type = substitute(expr->type);

	if (!is_inferred(expr->type)) {
		type_error(expr->op, "Unable to infer unary expression result type.");
	}
}

void TypeAnalyzer::substitute_binary_expression(const std::shared_ptr<BinaryExpression>& expr) {
	substitute_expression(expr->sides.first);
	substitute_expression(expr->sides.second);
	expr->type = substitute(expr->type);

	if (!is_inferred(expr->type)) {
		type_error(expr->op, "Unable to infer binary expression result type.");
	}
}

void TypeAnalyzer::substitute_block_expression(const std::shared_ptr<BlockExpression>& expr) {
	std::vector<ReturnExpression> rets{};
	for (auto& stmt : expr->statements) {
		substitute_statement(stmt);
		if (auto ret{std::dynamic_pointer_cast<ReturnExpression>(stmt)}) {
			rets.push_back(*ret);
		}
	}
	
	if (!rets.empty()) {
		if (!std::all_of(rets.begin(), rets.end(), [&](const auto& ret) { return ret.type == rets.front().type; })) {
			type_error(rets[0].return_tok, "All return types of a single block must be the same.");
			return;
		}
	}

	expr->type = substitute(expr->type);

	if (!is_inferred(expr->type)) {
		type_error(expr->opening_block, "Unable to infer block's type.");
	}
}

void TypeAnalyzer::substitute_call_expression(const std::shared_ptr<CallExpression>& expr) {
	expr->type = substitute(expr->type);
	for (auto& arg : expr->args) {
		substitute_expression(arg);

		if (!is_inferred(arg->type)) {
			type_error(expr->closing_paren, "Unable to infer function call argument type.");
		}
	}

	if (!is_inferred(expr->type)) {
		type_error(expr->closing_paren, "Unable to infer function call return type.");
	}
}

void TypeAnalyzer::substitute_return_expression(const std::shared_ptr<ReturnExpression>& expr) {
	substitute_expression(expr->return_expression);
	expr->type = substitute(expr->type);

	if (!is_inferred(expr->type)) {
		type_error(expr->return_tok, "Unable to infer return expression type.");
	}
}

void TypeAnalyzer::substitute_cast_expression(const std::shared_ptr<CastExpression>& expr) {
	substitute_expression(expr->expr);
	expr->type = substitute(expr->cast_type);
}

void TypeAnalyzer::substitute_statement(const std::shared_ptr<Statement>& stmt) {
	if (auto expr{std::dynamic_pointer_cast<ExpressionStatement>(stmt)}) {
		return substitute_expression_statement(expr);
	} else if (auto var_decl{std::dynamic_pointer_cast<VariableDeclarationStatement>(stmt)}) {
		return substitute_variable_declaration_statement(var_decl);
	} else if (auto func_decl{std::dynamic_pointer_cast<FunctionDeclarationStatement>(stmt)}) {
		return substitute_function_declaration_statement(func_decl);
	}
}

void TypeAnalyzer::substitute_expression_statement(const std::shared_ptr<ExpressionStatement>& stmt) {
	substitute_expression(stmt->expr);
}

void TypeAnalyzer::substitute_variable_declaration_statement(const std::shared_ptr<VariableDeclarationStatement>& stmt) {
	substitute_expression(stmt->initializer);
	stmt->type = substitute(stmt->type);

	if (!is_inferred(stmt->type)) {
		type_error(stmt->identifier->identifier, "Unable to infer variable type on declaration.");
	}
}

void TypeAnalyzer::substitute_function_declaration_statement(const std::shared_ptr<FunctionDeclarationStatement>& stmt) {
	stmt->return_type = substitute(stmt->return_type);
	for (auto& param : stmt->params) {
		param.first = substitute(param.first);

		if (!is_inferred(param.first)) {
			type_error(stmt->identifier->identifier, "Unable to infer function parameter type.");
		}
	}

	substitute_block_expression(stmt->block);

	if (!is_inferred(stmt->return_type)) {
		type_error(stmt->identifier->identifier, "Unable to infer function return type.");
	}
}

