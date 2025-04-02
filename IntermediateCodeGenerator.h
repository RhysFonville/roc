#pragma once

#include <memory>
#include <ranges>
#include <stack>
#include <set>
#include "Lexer.h"
#include "Syntax.h"
#include "Environment.h"
#include "Types.h"

constexpr uint8_t SZ_R{8u};
constexpr uint8_t SZ_E{4u};
constexpr uint8_t SZ_X{2u};
constexpr uint8_t SZ_H{1u};
constexpr uint8_t SZ_L{1u};

enum class IRCommandType {
	MOVE,
	ADD,
	SUB,
	MULT,
	DIV,
	XOR,
	NEG,
	CALL,
	RET,
	FUNC,
	LABEL,
	PUSH,
	POP,
	LEA,
	DIRECTIVE,
	LEAVE
};

namespace DIRECTIVES {
	const std::string ZSTR{"asciz"};
};

enum class RegisterName {
	Ret, CP1, Arg4, Arg3, Arg2,
	Arg1, Arg5, Arg6, GP1, GP2,
	CP2, CP3, CP4, CP5,
	Stack, Base, Instruction
}; 

static const std::vector<RegisterName> arg_regs{
	RegisterName::Arg1, RegisterName::Arg2, RegisterName::Arg3,
	RegisterName::Arg4, RegisterName::Arg5, RegisterName::Arg6
};

struct Register {
	Register() { }
	Register(RegisterName name, bool important)
		: name{name}, in_use{important}, important{important} { }
	Register(RegisterName name) : Register{name, false} { }

	bool operator==(const Register& reg) const noexcept { return reg.name == name; }

	RegisterName name{};
	bool in_use{};
	bool important{};
};

Register* occupy_next_reg(bool include_important = false);
Register* occupy_next_arg_reg();
Register* get_next_reg(bool occupy = false, bool include_important = false);
Register* occupy_reg(const RegisterName& name);
Register* get_reg(const RegisterName& name, bool occupy = false);

static std::shared_ptr<TConstructor> create_sz(TypeEnum t) {
	return std::make_shared<TConstructor>(types.at(t));
}

struct ASMValHolder {
	ASMValHolder() { }
	ASMValHolder(const Type& type) : held_type{type} { }

	Type held_type{};

	virtual void print(std::ostream& os = std::cout) const noexcept = 0;
};

struct ASMValRegister : public ASMValHolder {
	ASMValRegister() { }
	ASMValRegister(const Type& held_type, const std::optional<int>& offset)
		: ASMValHolder{held_type}, reg{get_reg(RegisterName::Base)},
		reg_size{SZ_R}, offset{offset} { }
	ASMValRegister(const Type& held_type, Register* reg)
		: ASMValHolder{held_type}, reg{reg}, reg_size{held_type->get_size()} { }
	ASMValRegister(const Type& held_type, Register* reg, bool dereferenced)
		: ASMValHolder{held_type}, reg{reg},
		dereferenced{dereferenced}, reg_size{SZ_R} { }

	Register* reg{};
	uint8_t reg_size{};
	std::optional<int> offset{std::nullopt};
	bool dereferenced{};

	bool operator==(const ASMValRegister& reg) const noexcept {
		return (comp_types(held_type, reg.held_type) && *this->reg == *reg.reg &&
				reg_size == reg.reg_size && offset == offset &&
				dereferenced == reg.dereferenced);
	}

	void print(std::ostream& os) const noexcept override {
		static std::vector<std::string> reg_strs{
			"RET", "ARG1", "ARG2", "ARG3", "ARG4", "ARG5", "ARG6",
			"CP1", "CP2", "CP3", "CP4", "CP5",
			"GP1", "GP2",
			"STACK", "BASE", "INSTRUCTION"
		};
		os << "% (sz" << reg_size << ")" << reg_strs[(size_t)reg->name];
		if (offset.has_value()) os << "-" << offset.value();
		os << std::endl;
	}
};

struct ASMValNonRegister : public ASMValHolder {
	ASMValNonRegister() { }
	ASMValNonRegister(const Type& held_type, const std::string& value)
		: ASMValHolder{held_type}, value{value} { }

	std::string value{};

	void print(std::ostream& os) const noexcept override {
		os << value << std::endl;
	}

	bool operator==(const ASMValNonRegister& non) const noexcept {
		return (comp_types(held_type, non.held_type) && value == non.value);
	}
};

using ASMVal = std::shared_ptr<ASMValHolder>;

static bool comp_asm_val(const ASMVal& lhs, const ASMVal& rhs) noexcept {
	if (auto lhs_reg{std::dynamic_pointer_cast<ASMValRegister>(lhs)}) {
		if (auto rhs_reg{std::dynamic_pointer_cast<ASMValRegister>(rhs)}) {
			return *lhs_reg == *rhs_reg;
		} else {
			return false;
		}
	} else {
		if (auto rhs_non{std::dynamic_pointer_cast<ASMValNonRegister>(rhs)}) {
			return *std::dynamic_pointer_cast<ASMValNonRegister>(lhs) == *rhs_non;
		} else {
			return false;
		}
	}
}

static std::vector<Register> registers{ // Can't be const
	{RegisterName::Ret},
	{RegisterName::Arg1}, {RegisterName::Arg2}, {RegisterName::Arg3}, {RegisterName::Arg4}, {RegisterName::Arg5}, {RegisterName::Arg6},
	{RegisterName::CP1}, {RegisterName::CP2}, {RegisterName::CP3}, {RegisterName::CP4}, {RegisterName::CP5},
	{RegisterName::GP1}, {RegisterName::GP2},
	{RegisterName::Stack, true}, {RegisterName::Base, true}, {RegisterName::Instruction, true},
};

struct IRCommand {
	IRCommandType type{};
	std::tuple<std::optional<ASMVal>, std::optional<ASMVal>, std::optional<ASMVal>> args{};

	inline friend std::ostream& operator<<(std::ostream& os, const IRCommand& cmd) noexcept;
};

inline std::ostream& operator<<(std::ostream& os, const IRCommand& cmd) noexcept {
	static std::vector<std::string> cmd_strs{
		"MOVE", "ADD", "SUB", "MULT", "DIV", "GOTO", "XOR", "NEG", "CALL", "RET", "FUNC", "LABEL", "PUSH", "POP"
	};
	static std::vector<std::string> reg_strs{
		"RET", "ARG1", "ARG2", "ARG3", "ARG4", "ARG5", "ARG6",
		"CP1", "CP2", "CP3", "CP4", "CP5",
		"GP1", "GP2",
		"Stack", "Base", "Instruction"
	};

	os << cmd_strs[(int)cmd.type] << " ";

	if (std::get<0>(cmd.args).has_value()) std::get<0>(cmd.args).value()->print(os);
	if (std::get<1>(cmd.args).has_value()) { os << ", "; std::get<1>(cmd.args).value()->print(os); }
	
	return os;
}

void unoccupy_if_reg(ASMVal& value);

static int ceiling_multiple(int number, int multiple) {
	int ret{(int)ceil((double)std::abs(number) / multiple) * multiple};
	if (number > 0) return ret;
	else return -ret;
}

class IntermediateCodeGenerator {
public:
	IntermediateCodeGenerator(const std::vector<std::shared_ptr<Statement>>& stmts)
		: stmts{stmts} { }

	std::vector<IRCommand> run();

private:
	std::vector<std::shared_ptr<Statement>> stmts{};
	std::vector<IRCommand> commands{};
	size_t commands_insert{0};

	std::vector<size_t> insert_jumps{};

	bool is_signed(const Type& t) {
		if (is_pointer(t)) return false;
		else return std::dynamic_pointer_cast<TConstructor>(t)->type.is_signed;
	}

	void insert_command(const IRCommand& command);
	void push_insert_spot(size_t insert);
	void pop_insert_spot();

	int create_var(const std::string& identifier, const Type& type, bool neg = true);

	std::string mangle_type(const std::string& t);
	std::string mangle_function(const std::shared_ptr<FunctionDeclarationStatement>& func);

	ASMVal generate_expression(const std::shared_ptr<Expression>& expr);
	ASMVal identifier_expression(const std::shared_ptr<IdentifierExpression>& expr);
	ASMVal literal_expression(const std::shared_ptr<LiteralExpression>& expr);
	ASMVal grouping_expression(const std::shared_ptr<GroupingExpression>& expr);
	ASMVal unary_expression(const std::shared_ptr<UnaryExpression>& expr);
	ASMVal binary_expression(const std::shared_ptr<BinaryExpression>& expr);
	ASMVal block_expression(const std::shared_ptr<BlockExpression>& expr, const std::shared_ptr<FunctionDeclarationStatement>& func = nullptr);
	ASMVal call_expression(const std::shared_ptr<CallExpression>& expr);
	ASMVal return_expression(const std::shared_ptr<ReturnExpression>& expr, const std::shared_ptr<FunctionDeclarationStatement>& func = nullptr);
	ASMVal cast_expression(const std::shared_ptr<CastExpression>& expr);

	void generate_statement(const std::shared_ptr<Statement>& stmt);
	void expression_statement(const std::shared_ptr<ExpressionStatement>& stmt);
	void variable_declaration_statement(const std::shared_ptr<VariableDeclarationStatement>& stmt);
	void function_declaration_statement(const std::shared_ptr<FunctionDeclarationStatement>& stmt);

	std::stack<std::string> env_stack{};
	std::map<std::string, std::string> funcs{};

	struct StackVar {
		std::string name{};
		Type type{};
		int pos{};
	};

	struct Stack {
		int pos_size{16};
		int neg_size{};
		std::vector<StackVar> vars{};
		bool call_function{};
	};
	std::stack<Stack> stacks{};

	const std::string BLOCK_ENV_STACK_NAME{"%_"};
	unsigned long long block_index{0};

	unsigned long long str_count{0};
};

