#include "ARM64CodeGenerator.h"
#include "IntermediateCodeGenerator.h"
#include "Lexer.h"
#include <memory>
#include <optional>

std::string ARM64CodeGenerator::asm_val_str(const ASMVal& val) const {
	if (auto reg{std::dynamic_pointer_cast<ASMValRegister>(val)}) {
		std::string ret{};
		if (reg->offset.has_value()) ret += "[" + std::to_string(reg->offset.value()) + ", ";
		ret += arm_registers[(size_t)reg->reg->name].sizes.at(reg->reg_size);
		if (reg->offset.has_value()) ret += "]";
		return ret;
	} else {
		return "#" + std::dynamic_pointer_cast<ASMValNonRegister>(val)->value;
	}
}

std::string ARM64CodeGenerator::basic_translation(const IRCommand& command) {
	std::string ret{arm_cmds.at(command.type) + " "};
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
	if (auto reg{std::dynamic_pointer_cast<ASMValRegister>(std::get<1>(command.args).value())}) {
		if (reg->reg->name == RegisterName::Stack) {
			auto reg{std::make_shared<ASMValRegister>(std::get<0>(command.args).value()->held_type, occupy_next_reg())};
			move(IRCommand{IRCommandType::MOVE, std::make_tuple(
				reg,
				std::get<1>(command.args).value(),
				std::nullopt
			)});
			asm_out.push_back("str " + asm_val_str(reg) + ", " + asm_val_str(std::get<0>(command.args).value()));
			reg->reg->in_use = false;
			return;
		}
	}
	asm_out.push_back(basic_translation(command));
}

void ARM64CodeGenerator::add(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ARM64CodeGenerator::sub(const IRCommand& command) {
	if (auto reg{std::dynamic_pointer_cast<ASMValRegister>(std::get<0>(command.args).value())}) {
		if (reg->reg->name == RegisterName::Stack) {
			stack_sub = std::stoi(std::dynamic_pointer_cast<ASMValNonRegister>(std::get<2>(command.args).value())->value);
		}
	}
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

}
void ARM64CodeGenerator::ret(const IRCommand& command) {
	asm_out.push_back(arm_cmds.at(command.type));
}
void ARM64CodeGenerator::func(const IRCommand& command) {
	asm_out.push_back(".global " + std::dynamic_pointer_cast<ASMValNonRegister>(get_first(command).value())->value);
	asm_out.push_back(std::dynamic_pointer_cast<ASMValNonRegister>(get_first(command).value())->value + ":");
}
void ARM64CodeGenerator::push(const IRCommand& command) {

}
void ARM64CodeGenerator::pop(const IRCommand& command) {

}
void ARM64CodeGenerator::lea(const IRCommand& command) {

}
void ARM64CodeGenerator::label(const IRCommand& command) {
	asm_out.push_back(std::dynamic_pointer_cast<ASMValNonRegister>(get_first(command).value())->value + ":");
}
void ARM64CodeGenerator::directive(const IRCommand& command) {
	asm_out.push_back(
		"." +
		std::dynamic_pointer_cast<ASMValNonRegister>(get_first(command).value())->value +
		" " +
		std::dynamic_pointer_cast<ASMValNonRegister>(get_second(command).value())->value
	);
}
void ARM64CodeGenerator::leave(const IRCommand& command) {
	ASMValRegister sp{create_sz(TypeEnum::U64), get_reg(RegisterName::Stack)};
	add(IRCommand{IRCommandType::ADD, std::make_tuple(
		std::make_shared<ASMValRegister>(sp),
		std::make_shared<ASMValRegister>(sp),
		std::make_shared<ASMValNonRegister>(create_sz(TypeEnum::U64), std::to_string(stack_sub))
	)});
}

