#include <algorithm>
#include <memory>
#include <optional>
#include "IntermediateCodeGenerator.h"
#include "Lexer.h"
#include "Syntax.h"
#include "Types.h"

void IntermediateCodeGenerator::insert_command(const IRCommand& command) {
	commands.insert(commands.begin()+commands_insert++, command);
	for (size_t& jump : insert_jumps) {
		if (jump > commands_insert-1) {
	  		jump++;
		}
	}
}

void IntermediateCodeGenerator::push_insert_spot(size_t insert) {
	insert_jumps.push_back(commands_insert);
	commands_insert = insert;
}

void IntermediateCodeGenerator::pop_insert_spot() {
	commands_insert = insert_jumps.back();
	insert_jumps.pop_back();
}

inline Register* get_next_reg(bool occupy, bool include_important) {
	for (Register& reg : registers) {
		if (!include_important && reg.important) continue;
		if (!reg.in_use) {
			if (occupy) reg.in_use = true;
			return &reg;
		}
	}

	return nullptr;
}

inline Register* occupy_next_reg(bool include_important) {
	return get_next_reg(true, include_important);
}

inline Register* occupy_next_arg_reg() {
	for (Register& reg : registers) {
		if (std::ranges::find(arg_regs, reg.name) == arg_regs.end()) continue;
		if (!reg.in_use) {
			reg.in_use = true;
			return &reg;
		}
	}

	return nullptr;
}

inline Register* occupy_reg(const RegisterName& name) {
	return get_reg(name, true);
}

inline Register* get_reg(const RegisterName& name, bool occupy) {
	for (Register& reg : registers) {
		if (reg.name == name) {
			reg.in_use = true;
			return &reg;
		}
	}
	return nullptr;
}

inline void unoccupy_if_reg(ASMVal& value) {
	if (auto reg{std::dynamic_pointer_cast<ASMValRegister>(value)}) {
		reg->reg->in_use = false;
	}
}

int IntermediateCodeGenerator::create_var(const std::string& identifier, const Type& type, bool neg) {
	int offset{};
	if (neg) {
		stacks.top().neg_size -= type->get_size();
		stacks.top().neg_size = ceiling_multiple(stacks.top().neg_size, type->get_size());
		offset = stacks.top().neg_size;
	} else {
		offset = stacks.top().pos_size;
		stacks.top().pos_size += SZ_R;
	}

	stacks.top().vars.push_back({identifier, type, offset});
	return offset;
}

std::string IntermediateCodeGenerator::mangle_type(const std::string& t) {
	if (t == "int") {
		return "i";
	} if (t == "str") {
		return "s";
	} if (t == "bool") {
		return "b";
	} if (t == "none") {
		return "n";
	} else {
		return "";
	}
}

std::string IntermediateCodeGenerator::mangle_function(const std::shared_ptr<FunctionDeclarationStatement>& func) {
	std::string name{"_Z"};
	std::stack<std::string> env_stack_copy{env_stack};
	
	if (!env_stack_copy.empty()) {
		name += "N";
		do {
			if (env_stack.top() == BLOCK_ENV_STACK_NAME) {
				name += env_stack.top().substr(1);
			} else {
				name += std::to_string(env_stack_copy.top().size()) + env_stack_copy.top();
			}
			env_stack_copy.pop();
		} while (!env_stack_copy.empty());
		name += std::to_string(func->identifier->identifier.value.size()) + func->identifier->identifier.value;
		name += "E";
	} else {
		name += std::to_string(func->identifier->identifier.value.size()) + func->identifier->identifier.value;
	}

	/*if (func->params.empty()) {
		name += mangle_type("none");
	} else {
		for (const std::pair<Token, Token>& param : func->params) {
			name += mangle_type(param.first.value);
		}
	}*/
	
	return name;
}

std::vector<IRCommand> IntermediateCodeGenerator::run() {
	for (const auto& stmt : stmts) {
		generate_statement(stmt);
	}
	return commands;
}

ASMVal IntermediateCodeGenerator::generate_expression(const std::shared_ptr<Expression>& expr) {
	if (auto id{std::dynamic_pointer_cast<IdentifierExpression>(expr)}) {
		return identifier_expression(id);
	} else if (auto lit{std::dynamic_pointer_cast<LiteralExpression>(expr)}) {
		return literal_expression(lit);
	} else if (auto group{std::dynamic_pointer_cast<GroupingExpression>(expr)}) {
		return grouping_expression(group);
	} else if (auto unary{std::dynamic_pointer_cast<UnaryExpression>(expr)}) {
		return unary_expression(unary);
	} else if (auto binary{std::dynamic_pointer_cast<BinaryExpression>(expr)}) {
		return binary_expression(binary);
	} else if (auto block{std::dynamic_pointer_cast<BlockExpression>(expr)}) {
		return block_expression(block);
	} else if (auto call{std::dynamic_pointer_cast<CallExpression>(expr)}) {
		return call_expression(call);
	} else if (auto ret{std::dynamic_pointer_cast<ReturnExpression>(expr)}) {
		return return_expression(ret);
	} else if (auto cast{std::dynamic_pointer_cast<CastExpression>(expr)}) {
		return cast_expression(cast);
	}

	return nullptr;
}

ASMVal IntermediateCodeGenerator::identifier_expression(const std::shared_ptr<IdentifierExpression>& expr) {
	if (auto it{std::ranges::find_if(stacks.top().vars, [&](auto v){return v.name==expr->identifier.value;})}; it != stacks.top().vars.end()) {
		auto reg{std::make_shared<ASMValRegister>(it->type, it->pos)};
		insert_command(IRCommand{IRCommandType::LOAD, std::make_tuple(
			reg, std::nullopt, std::nullopt
		)});
		return reg;
	} else {
		return std::make_shared<ASMValNonRegister>(it->type, expr->identifier.value);
	}
}

ASMVal IntermediateCodeGenerator::literal_expression(const std::shared_ptr<LiteralExpression>& expr) {
	if (expr->value.type == TokenType::TRUE) {
		return std::make_shared<ASMValNonRegister>(expr->type, "1");
	} else if (expr->value.type == TokenType::FALSE) {
		return std::make_shared<ASMValNonRegister>(expr->type, "0");
	} else if (expr->value.type == TokenType::CHAR_LITERAL) {
		return std::make_shared<ASMValNonRegister>(expr->type, std::to_string((int)expr->value.value[0]));
	} else if (expr->value.type == TokenType::STRING_LITERAL) {
		push_insert_spot(0);
		auto str{std::make_shared<ASMValNonRegister>(expr->type, ".STR" + std::to_string(str_count++))};
		insert_command(IRCommand{IRCommandType::LABEL, std::make_tuple(
			str, std::nullopt, std::nullopt
		)});
		insert_command(IRCommand{IRCommandType::DIRECTIVE, std::make_tuple(
			std::make_shared<ASMValNonRegister>(nullptr, DIRECTIVES::ZSTR),
			std::make_shared<ASMValNonRegister>(nullptr, "\"" + expr->value.value + "\""),
			std::nullopt
		)});
		pop_insert_spot();
		return str;
	} else if (expr->value.type == TokenType::NUMBER_LITERAL) {
		auto end{std::ranges::find_if(expr->value.value, [](char c) { return std::isalpha(c); })};
		if (end != expr->value.value.end()) {
			return std::make_shared<ASMValNonRegister>(expr->type, std::string{expr->value.value.begin(), end});
		}
	}
	
	return std::make_shared<ASMValNonRegister>(expr->type, expr->value.value);
}

ASMVal IntermediateCodeGenerator::grouping_expression(const std::shared_ptr<GroupingExpression>& expr) {
	return generate_expression(expr->expr);
}

ASMVal IntermediateCodeGenerator::unary_expression(const std::shared_ptr<UnaryExpression>& expr) {
	auto rhs{generate_expression(expr->expr)};
	auto reg{occupy_next_reg()};
	if (expr->op.type == TokenType::NOT) {
		insert_command(IRCommand{IRCommandType::XOR, std::make_tuple(
			std::make_shared<ASMValRegister>(rhs->held_type, reg),
			rhs,
			std::make_shared<ASMValNonRegister>(rhs->held_type, "1")
		)});
	} else if (expr->op.type == TokenType::MINUS) {
		insert_command(IRCommand{IRCommandType::NEG, std::make_tuple(
			std::make_shared<ASMValRegister>(rhs->held_type, reg),
			rhs,
			std::nullopt
		)});
	} else if (expr->op.type == TokenType::AMPERSAND) {
		auto ret{std::make_shared<ASMValRegister>(expr->type, reg)};
		insert_command(IRCommand{IRCommandType::LEA, std::make_tuple(
			ret, rhs, std::nullopt
		)});

		unoccupy_if_reg(rhs);
		return ret;
	} else if (expr->op.type == TokenType::STAR) {
		ASMValRegister deref_reg{expr->type, reg, true};
		deref_reg.dereferenced = false;
		insert_command(IRCommand{IRCommandType::MOVE, std::make_tuple(
			std::make_shared<ASMValRegister>(deref_reg), rhs, std::nullopt
		)});
		deref_reg.dereferenced = true;

		unoccupy_if_reg(rhs);
		return std::make_shared<ASMValRegister>(deref_reg);
	}

	unoccupy_if_reg(rhs);
	return std::make_shared<ASMValRegister>(rhs->held_type, reg);
}
ASMVal IntermediateCodeGenerator::binary_expression(const std::shared_ptr<BinaryExpression>& expr) {
	ASMVal lhs{generate_expression(expr->sides.first)};
	ASMVal rhs{generate_expression(expr->sides.second)};
	switch (expr->op.type) {
		case TokenType::PLUS:
		case TokenType::MINUS:
		case TokenType::STAR:
		case TokenType::SLASH: {
			// If literal (op) literal
			if (auto lhs_non{std::dynamic_pointer_cast<ASMValNonRegister>(lhs)}) {
				if (auto rhs_non{std::dynamic_pointer_cast<ASMValNonRegister>(rhs)}) {
					auto execute_op{[&](auto x, auto y, TokenType op) {
						if (expr->op.type == TokenType::PLUS) return x + y;
						if (expr->op.type == TokenType::MINUS) return x - y;
						if (expr->op.type == TokenType::STAR) return x * y;
						if (expr->op.type == TokenType::SLASH) return x / y;
						else return x;
					}};
					std::string asm_val{std::to_string(execute_op(
						is_signed(lhs_non->held_type) ? (long long)std::stoll(lhs_non->value) : (unsigned long long)std::stoull(lhs_non->value),
						is_signed(rhs_non->held_type) ? (long long)std::stoll(rhs_non->value) : (unsigned long long)std::stoull(rhs_non->value),
						expr->op.type
					))};
					return std::make_shared<ASMValNonRegister>(lhs->held_type, asm_val);
				}
			}

			IRCommandType type{};
			if (expr->op.type == TokenType::PLUS) type = IRCommandType::ADD;
			if (expr->op.type == TokenType::MINUS) type = IRCommandType::SUB;
			if (expr->op.type == TokenType::STAR) type = IRCommandType::MULT;
			if (expr->op.type == TokenType::SLASH) type = IRCommandType::DIV;

			if (auto lhs_reg{std::dynamic_pointer_cast<ASMValRegister>(lhs)}) {
				if (lhs_reg->offset.has_value()) {
					auto reg{std::make_shared<ASMValRegister>(lhs->held_type, occupy_next_reg())};
					insert_command(IRCommand{type, std::make_tuple(reg, lhs, rhs)});
					unoccupy_if_reg(lhs);
					unoccupy_if_reg(rhs);
					return reg;
				} else {
					insert_command(IRCommand{type, std::make_tuple(lhs, lhs, rhs)});
					unoccupy_if_reg(lhs);
					unoccupy_if_reg(rhs);
					return lhs;
				}
			}
		}
		case TokenType::EQUAL:
			insert_command(IRCommand{IRCommandType::MOVE, std::make_tuple(lhs, rhs, std::nullopt)});
			break;
		default:
			break;
	}

	unoccupy_if_reg(rhs);
	return lhs;
}

ASMVal IntermediateCodeGenerator::block_expression(const std::shared_ptr<BlockExpression>& expr, const std::shared_ptr<FunctionDeclarationStatement>& func) {
	if (func != nullptr) {
		push_insert_spot(commands_insert);
		stacks.push(Stack{0, 0, {}});
			
		std::vector<Register*> regs{};
		for (const auto& param : func->params) {
			Register* reg{occupy_next_arg_reg()};
			if (reg != nullptr) {
				int offset{create_var(param.second.value, param.first)};
				insert_command(IRCommand{IRCommandType::MOVE, std::make_tuple(
					std::make_shared<ASMValRegister>(param.first, offset),
					std::make_shared<ASMValRegister>(param.first, reg),
					std::nullopt
				)});
				regs.push_back(reg);
			} else {
				if (stacks.top().pos_size == 0) stacks.top().pos_size += 16;
				create_var(param.second.value, param.first, false);
			}
		}
		for (Register* reg : regs) reg->in_use = false;
	}

	env_stack.push(func == nullptr ? "_" + std::to_string(block_index++)
				: func->identifier->identifier.value);

	ASMVal ret_val{};
	for (const std::shared_ptr<Statement>& stmt : expr->statements) {
		if (auto expr_stmt{std::dynamic_pointer_cast<ExpressionStatement>(stmt)}) {
			if (auto ret{std::dynamic_pointer_cast<ReturnExpression>(expr_stmt->expr)}) {
	  			ret_val = return_expression(ret, func);
	  		} else {
				generate_expression(expr_stmt->expr);
			}
		} else {
			generate_statement(stmt);

		}
	}

	if (func != nullptr) {
		if (ret_val == nullptr) {
			return_expression(std::make_shared<ReturnExpression>(Token{}, nullptr), func);
		}

		pop_insert_spot();
		int sub{-stacks.top().neg_size};
		if (sub > 128 && !stacks.top().call_function) { // Red zone
			sub = ceiling_multiple(sub - 128, 8);
		} else if (stacks.top().call_function) {
			sub = ceiling_multiple(sub, 16);
		} else if (!stacks.top().vars.empty()) {
			sub = std::max(16, sub);
		}
		if (sub != 0) {
			static auto stack_reg = ASMValRegister{
				create_sz(TypeEnum::U64), get_reg(RegisterName::Stack)
			};
			insert_command(IRCommand{IRCommandType::SUB, std::make_tuple(
				std::make_shared<ASMValRegister>(stack_reg),
				std::make_shared<ASMValRegister>(stack_reg),
				std::make_shared<ASMValNonRegister>(
					create_sz(TypeEnum::U64), std::to_string(sub)
				)
			)});
		}

		stacks.pop();
	}

	env_stack.pop();

	return ret_val;
}

ASMVal IntermediateCodeGenerator::call_expression(const std::shared_ptr<CallExpression>& expr) {
	stacks.top().call_function = true;

	std::vector<Register*> regs{};
	int pushed_size{};
	int push_count{};
	int first_push_i{-1};
	std::vector<ASMVal> arg_vals{};
	arg_vals.reserve(expr->args.size());
	for (int i{0}; i < expr->args.size(); i++) {
		ASMVal arg_val{generate_expression(expr->args[i])};
		arg_vals.push_back(arg_val);

		Register* reg{occupy_next_arg_reg()};
		if (reg != nullptr) {
			Type mv_type{expr->args[i]->type};
			if (mv_type->get_size() < SZ_E) mv_type = create_sz(TypeEnum::U32);
			insert_command(IRCommand{IRCommandType::MOVE, std::make_tuple(
				std::make_shared<ASMValRegister>(expr->args[i]->type, reg),
				arg_val,
				std::nullopt
			)});
			regs.push_back(reg);
		} else {
			if (first_push_i == -1) first_push_i = i;
			push_count++;
		}
	}
	for (size_t i{expr->args.size()-1}; i >= first_push_i; i--) {
		if (auto reg{std::dynamic_pointer_cast<ASMValRegister>(arg_vals[i])}) {
			reg->reg_size = SZ_R;
		} else {
			std::dynamic_pointer_cast<ASMValNonRegister>(arg_vals[i])->held_type = create_sz(TypeEnum::U64);
		}
		insert_command(IRCommand{IRCommandType::PUSH, std::make_tuple(arg_vals[i], std::nullopt, std::nullopt)});
		pushed_size += arg_vals[i]->held_type->get_size();
	}
	for (Register* reg : regs) reg->in_use = false;

	std::string name{std::dynamic_pointer_cast<IdentifierExpression>(expr->callee)->identifier.value};
	std::vector<Type> args{
		expr->args
			| std::views::transform([](auto arg){return arg->type;})
			| std::ranges::to<std::vector>()
	};

	bool is_native_func{
		std::ranges::find_if(NATIVE_FUNCTIONS, [&](const Function& native_func){
			return native_func.is_func(expr->type, name, args);
		}) != NATIVE_FUNCTIONS.end()
	};
	if (!is_native_func) {
		name = {funcs[name]};
	}

	insert_command(IRCommand{IRCommandType::CALL, std::make_tuple(
		std::make_shared<ASMValNonRegister>(expr->type, name),
		std::nullopt,
		std::nullopt
	)});
	
	if (pushed_size > 0) {
		static auto stack_reg{ASMValRegister{create_sz(TypeEnum::U64), get_reg(RegisterName::Stack)}};
		insert_command(IRCommand{IRCommandType::ADD, std::make_tuple(
			std::make_shared<ASMValRegister>(stack_reg),
			std::make_shared<ASMValRegister>(stack_reg),
			std::make_shared<ASMValNonRegister>(create_sz(TypeEnum::U64), std::to_string(pushed_size))
		)});
	}

	return std::make_shared<ASMValRegister>(expr->type, occupy_reg(RegisterName::Ret));
}

ASMVal IntermediateCodeGenerator::return_expression(const std::shared_ptr<ReturnExpression>& expr, const std::shared_ptr<FunctionDeclarationStatement>& func) {
	if (expr->return_expression != nullptr) {
		Type mv_type{expr->type};
		if (mv_type->get_size() < SZ_E) mv_type = create_sz(TypeEnum::U32);
		insert_command(IRCommand{IRCommandType::MOVE, std::make_tuple(
			std::make_shared<ASMValRegister>(mv_type, occupy_reg(RegisterName::Ret)),
			generate_expression(expr->return_expression),
			std::nullopt
		)});
	}

	if (func != nullptr) {
		if (stacks.top().vars.empty()) {
			insert_command(IRCommand{IRCommandType::POP, std::make_tuple(
				std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), occupy_reg(RegisterName::Base), false),
				std::nullopt,
				std::nullopt
			)});
		} else {
			insert_command(IRCommand{IRCommandType::LEAVE,
				std::make_tuple(std::nullopt, std::nullopt, std::nullopt)
			});
		}
		insert_command(IRCommand{IRCommandType::RET, std::make_tuple(std::nullopt, std::nullopt, std::nullopt)});
	}

	if (expr->return_expression != nullptr)
		return std::make_shared<ASMValRegister>(expr->type, occupy_reg(RegisterName::Ret));
	else
		return nullptr;
}

ASMVal IntermediateCodeGenerator::cast_expression(const std::shared_ptr<CastExpression>& expr) {
	auto ret{generate_expression(expr->expr)};
	ret->held_type = expr->type;
	return ret;
}

void IntermediateCodeGenerator::generate_statement(const std::shared_ptr<Statement>& stmt) {
	if (auto decl{std::dynamic_pointer_cast<VariableDeclarationStatement>(stmt)}) {
		variable_declaration_statement(decl);
	} else if (auto decl{std::dynamic_pointer_cast<FunctionDeclarationStatement>(stmt)}) {
		function_declaration_statement(decl);
	} else if (auto expr{std::dynamic_pointer_cast<ExpressionStatement>(stmt)}) {
		expression_statement(expr);
	}
	for (Register& reg : registers) reg.in_use = false;
}

void IntermediateCodeGenerator::expression_statement(const std::shared_ptr<ExpressionStatement>& stmt) {
	generate_expression(stmt->expr);
}

void IntermediateCodeGenerator::variable_declaration_statement(const std::shared_ptr<VariableDeclarationStatement>& stmt) {
	/*insert_command(
		IRCommand{
			IRCommandType::MOVE,
			std::make_pair(
				generate_expression(stmt->initializer),
				ASMValNonRegister{stmt->identifier->type.size, stmt->identifier->identifier.value}
			)
		}
	);*/
	auto offset{create_var(stmt->identifier->identifier.value, stmt->type)};
	insert_command(IRCommand{IRCommandType::LOAD, std::make_tuple(
		std::make_shared<ASMValRegister>(stmt->type, offset),
		generate_expression(stmt->initializer),
		std::nullopt
	)});
}

void IntermediateCodeGenerator::function_declaration_statement(const std::shared_ptr<FunctionDeclarationStatement>& stmt) {
	std::string name{mangle_function(stmt)};
	if (stmt->identifier->identifier.value == "main") {
		name = stmt->identifier->identifier.value;	
	}
	funcs.insert(std::make_tuple(stmt->identifier->identifier.value, name));

	push_insert_spot(0);

	insert_command(IRCommand{IRCommandType::FUNC, std::make_tuple(
		std::make_shared<ASMValNonRegister>(stmt->return_type, name),
		std::nullopt,
		std::nullopt
	)});
	insert_command(IRCommand{IRCommandType::PUSH, std::make_tuple(
		std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), occupy_reg(RegisterName::Base)),
		std::nullopt,
		std::nullopt
	)});
	insert_command(IRCommand{IRCommandType::MOVE, std::make_tuple(
		std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), occupy_reg(RegisterName::Base)),
		std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), occupy_reg(RegisterName::Stack)),
		std::nullopt
	)});

	block_expression(stmt->block, stmt);
	pop_insert_spot();
}

