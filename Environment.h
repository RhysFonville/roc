#pragma once

#include <set>
#include <ranges>
#include "Types.h"

struct Variable {
	MixType type{};
	Token name{};

	bool operator<(const Variable& var) const noexcept { return name < var.name; }
	bool operator==(const Token& var) const noexcept { return name.value == var.value; }
};

struct Function {
	MixType return_type{};
	Token name{};
	std::vector<Variable> args{};

	bool operator<(const Function& func) const noexcept { return name < func.name; }
	bool operator==(const Token& func) const noexcept { return (name.value == func.value); }
	bool operator==(const Function& func) const noexcept {
		bool name_and_ret{name.value == func.name.value && return_type == func.return_type};
	   	if (!name_and_ret) return false;
		
		if (args.size() != func.args.size()) return false;
		for (int i{0}; i < args.size(); i++) {
			if (args[i].type != func.args[i].type) return false;
		}

		return true;
	}

	bool is_func(const MixType& return_type, const std::string& name, const std::vector<MixType>& args) const noexcept {
		std::vector<Variable> var_args{
			args
			| std::views::transform([&](const MixType& t) -> Variable { return Variable{t, Token{"", TokenType::IDENTIFIER, 0u}}; })
			| std::ranges::to<std::vector>()
		};
		return *this == Function{return_type, Token{name}, var_args};
	}
};

const std::vector<Function> NATIVE_FUNCTIONS{
	Function{
		primitive_types.at(PrimitiveTypeEnum::NONE).to_mix_type(), {"write"}, {
			Variable{primitive_types.at(PrimitiveTypeEnum::I32).to_mix_type(), {"fd"}},
			Variable{primitive_types.at(PrimitiveTypeEnum::I8).to_mix_type(), {"buf"}},
			Variable{primitive_types.at(PrimitiveTypeEnum::I32).to_mix_type(), {"count"}}
		}
	}
};

struct Environment {
	std::vector<Variable> variables{};
	std::vector<Function> functions{};
	std::vector<MixType> types{};
};

struct EnvironmentStack {
	std::optional<Variable> get_variable(const Token& identifier) const noexcept {
		for (const auto& env : envs | std::views::reverse) {
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
		for (const auto& env : envs | std::views::reverse) {
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
	std::optional<MixType> get_identifier_type(const Token& identifier) const noexcept {
		if (auto var{get_variable(identifier)}) return var.value().type;
		else if (auto func{get_function(identifier)}) return func.value().return_type;
		else return std::nullopt;
	}
	
	std::optional<MixType> get_type(const Token& name) const noexcept {
		for (const auto& env : envs | std::views::reverse) {
			if (auto it{std::ranges::find_if(env.types, [&](const MixType& type) {
				return type.get_parse_type().get_base_type_token().value().value == name.value;
			})}; it != env.types.end()) {
				return *it;
			}
		}
		return std::nullopt;
	}

	Environment& back() { return envs.back(); }
	void push(const Environment& env) { envs.push_back(env); }
	void pop() { envs.pop_back(); }

	std::vector<Environment> envs{Environment{{}, NATIVE_FUNCTIONS, {
		primitive_types
			| std::views::values
			| std::views::transform([](const auto& t) { return t.to_mix_type(); })
			| std::ranges::to<std::vector>()
	}}};
};

