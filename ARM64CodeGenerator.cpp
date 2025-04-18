#include "ARM64CodeGenerator.h"
#include "IntermediateCodeGenerator.h"
#include "Lexer.h"
#include <memory>
#include <optional>
#include <regex>
#include <stack>

std::string ARM64CodeGenerator::asm_val_str(const ASMVal& val) const {
	if (auto reg{std::dynamic_pointer_cast<ASMValRegister>(val)}) {
		std::string ret{};
		if (reg->offset.has_value()) ret += "[";
		ret += arm_registers[(size_t)reg->reg->name].sizes.at(reg->reg_size);
		if (reg->offset.has_value()) ret += ", #" + std::to_string(reg->offset.value()) + "]";
		return ret;
	} else {
		return "#" + std::dynamic_pointer_cast<ASMValNonRegister>(val)->value;
	}
}

std::string ARM64CodeGenerator::basic_translation(const IRCommand& command, const std::string& diff_cmd) {
	std::string ret{(diff_cmd.empty() ? arm_cmds.at(command.type) : diff_cmd) + " "};
	if (auto arg1{std::get<0>(command.args)})
		ret += asm_val_str(arg1.value());
	if (auto arg2{std::get<1>(command.args)})
		ret += ", " + asm_val_str(arg2.value());
	if (auto arg3{std::get<2>(command.args)})
		ret += ", " + asm_val_str(arg3.value());
	return ret;
}

void ARM64CodeGenerator::preamble() {
	asm_out.insert(asm_out.begin(), ".section	__TEXT,__text,regular,pure_instructions");
}

void ARM64CodeGenerator::move(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ARM64CodeGenerator::add(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ARM64CodeGenerator::sub(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}
void ARM64CodeGenerator::mul(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}
void ARM64CodeGenerator::div(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}
void ARM64CodeGenerator::xor_cmd(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}
void ARM64CodeGenerator::neg(const IRCommand& command) {

}
void ARM64CodeGenerator::call(const IRCommand& command) {
	asm_out.push_back("bl " + std::dynamic_pointer_cast<ASMValNonRegister>(std::get<0>(command.args).value())->value);
}
void ARM64CodeGenerator::ret(const IRCommand& command) {
	asm_out.push_back(arm_cmds.at(IRCommandType::RET));
}
void ARM64CodeGenerator::func(const IRCommand& command) {
	asm_out.push_back(".global " + std::dynamic_pointer_cast<ASMValNonRegister>(get_first(command).value())->value);
	asm_out.push_back(std::dynamic_pointer_cast<ASMValNonRegister>(get_first(command).value())->value + ":");
}
void ARM64CodeGenerator::set_arg(const IRCommand& command) {

}
void ARM64CodeGenerator::enter_stack(const IRCommand& command) {
	auto stack_reg{get_reg(RegisterName::Stack)};
	auto base_reg{get_reg(RegisterName::Base)};

	bool called_func{(bool)std::stoi(std::dynamic_pointer_cast<ASMValNonRegister>(std::get<1>(command.args).value())->value)};
	auto sp_sub{std::max(16, ceiling_multiple(-std::stoi(std::dynamic_pointer_cast<ASMValNonRegister>(std::get<0>(command.args).value())->value), 16))};
	if (called_func) {
		sp_sub += 16;
	}

	sub(IRCommand{IRCommandType::SUB, std::make_tuple(
		std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), stack_reg),
		std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), stack_reg),
		std::make_shared<ASMValNonRegister>(create_sz(TypeEnum::U64), std::to_string(sp_sub))
	)});
	sp -= sp_sub;

	if (called_func) {
		sp_sub -= 16;
		store(IRCommand{IRCommandType::STORE, std::make_tuple(
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), base_reg),
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), get_reg(RegisterName::RetAddress)),
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), 16)
		)});
		add(IRCommand{IRCommandType::ADD, std::make_tuple(
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), base_reg),
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), stack_reg),
			std::make_shared<ASMValNonRegister>(create_sz(TypeEnum::U64), std::to_string(sp_sub))
		)});
	}
	bp = sp + 16;
}
void ARM64CodeGenerator::exit_stack(const IRCommand& command) {
	ASMValRegister stack_reg{create_sz(TypeEnum::U64), get_reg(RegisterName::Stack)};
	ASMValRegister base_reg{create_sz(TypeEnum::U64), get_reg(RegisterName::Base)};
	auto sp_add{std::max(16, ceiling_multiple(-std::stoi(std::dynamic_pointer_cast<ASMValNonRegister>(std::get<0>(command.args).value())->value), 16))};

	if ((bool)std::stoi(std::dynamic_pointer_cast<ASMValNonRegister>(std::get<1>(command.args).value())->value)) {
		sp_add += 16;
		store(IRCommand{IRCommandType::LOAD, std::make_tuple(
			std::make_shared<ASMValRegister>(base_reg),
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), get_reg(RegisterName::RetAddress)),
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), 16)
		)});
		bp += 16;
	}
	add(IRCommand{IRCommandType::ADD, std::make_tuple(
		std::make_shared<ASMValRegister>(stack_reg),
		std::make_shared<ASMValRegister>(stack_reg),
		std::make_shared<ASMValNonRegister>(create_sz(TypeEnum::U64), std::to_string(sp_add))
	)});
	sp += sp_add;
}
void ARM64CodeGenerator::label(const IRCommand& command) {
	asm_out.push_back(std::dynamic_pointer_cast<ASMValNonRegister>(get_first(command).value())->value + ":");
}
void ARM64CodeGenerator::push(const IRCommand& command) {

}
void ARM64CodeGenerator::pop(const IRCommand& command) {

}
void ARM64CodeGenerator::lea(const IRCommand& command) {

}
void ARM64CodeGenerator::directive(const IRCommand& command) {
	asm_out.push_back(
		"." +
		std::dynamic_pointer_cast<ASMValNonRegister>(get_first(command).value())->value +
		" " +
		std::dynamic_pointer_cast<ASMValNonRegister>(get_second(command).value())->value
	);
}
void ARM64CodeGenerator::store(const IRCommand& command) {
	std::string cmd{};
	if (std::get<2>(command.args).has_value()) cmd = "stp";
	else cmd = "str";
	asm_out.push_back(basic_translation(command, cmd));
}
void ARM64CodeGenerator::load(const IRCommand& command) {
	std::string cmd{};
	if (std::get<2>(command.args).has_value()) cmd = "ldp";
	else cmd = "ldr";
	asm_out.push_back(basic_translation(command, cmd));
}
void ARM64CodeGenerator::nothing(const IRCommand& command) {

}
void ARM64CodeGenerator::zero(const IRCommand& command) {

}
