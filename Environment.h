#pragma once

#include <set>
#include "Syntax.h"
#include "Types.h"

struct Variable {
	Type type{};
	Token name{};

	bool operator<(const Variable& var) const noexcept { return name < var.name; }
	bool operator==(const Token& var) const noexcept { return name.value == var.value; }
};

struct Function {
	Type return_type{};
	Token name{};
	std::vector<Variable> args{};

	bool operator<(const Function& func) const noexcept { return name < func.name; }
	bool operator==(const Token& func) const noexcept { return (name.value == func.value); }
	bool operator==(const Function& func) const noexcept {
		bool name_and_ret{name.value == func.name.value && comp_types(return_type, func.return_type)};
	   	if (!name_and_ret) return false;
		
		if (args.size() != func.args.size()) return false;
		for (int i{0}; i < args.size(); i++) {
			if (!comp_types(args[i].type, func.args[i].type)) return false;
		}

		return true;
	}

	bool is_func(const Type& return_type, const std::string& name, const std::vector<Type>& args) const noexcept {
		std::vector<Variable> var_args{
			args
			| std::views::transform([&](const Type& t) -> Variable { return Variable{t, Token{"", TokenType::IDENTIFIER, 0u}}; })
			| std::ranges::to<std::vector>()
		};
		return *this == Function{return_type, Token{name}, var_args};
	}
};

const std::set<Function> NATIVE_FUNCTIONS{
	Function{
		std::make_shared<TConstructor>(types.at(TypeEnum::NONE)), {"write"},
		{
			Variable{std::make_shared<TConstructor>(types.at(TypeEnum::I32)), {"fd"}},
			Variable{std::make_shared<TPointer>(std::make_shared<TConstructor>(types.at(TypeEnum::I8))), {"buf"}},
			Variable{std::make_shared<TConstructor>(types.at(TypeEnum::I32)), {"count"}}
		}
	}
};


struct Environment {
	std::set<Variable> variables{};
	std::set<Function> functions{};
};

struct EnvironmentStack {
	std::optional<Variable> get_variable(const Token& identifier) const noexcept {
		for (auto env : envs) {
			auto var{std::ranges::find_if(env.variables, [&](auto v){
				return v == identifier;
			})};
			if (var != env.variables.end()) {
				return *var;
			}
		}

		return std::nullopt;
	}
	std::optional<Function> get_function(const Token& identifier) const noexcept {
		for (auto env : envs) {
			auto func{std::ranges::find_if(env.functions, [&](auto f){
				return f == identifier;
			})};
			if (func != env.functions.end()) {
				return *func;
			}
		}

		return std::nullopt;
	}
	bool has_identifier(const Token& identifier) const noexcept {
		if (get_variable(identifier)) return true;
		else return get_function(identifier).has_value();
	}
	std::optional<Type> get_identifier_type(const Token& identifier) const noexcept {
		if (auto var{get_variable(identifier)}) return var.value().type;
		else if (auto func{get_function(identifier)}) return func.value().return_type;
		else return std::nullopt;
	}

	Environment& back() { return envs.back(); }
	void push(const Environment& env) { envs.push_back(env); }
	void pop() { envs.pop_back(); }

	std::vector<Environment> envs{Environment{{}, NATIVE_FUNCTIONS}};
};

