#include "ARM64CodeGenerator.h"
#include "IntermediateCodeGenerator.h"
#include "Lexer.h"
#include "MachineSpecificCodeGenerator.h"
#include <algorithm>
#include <memory>
#include <optional>

std::string ARM64CodeGenerator::asm_val_str(const ASMVal& val) const {
	if (auto reg{std::dynamic_pointer_cast<ASMValRegister>(val)}) {
		if (reg->offset.has_value()) {
			return "[" + arm_registers[(size_t)RegisterName::Stack].sizes.at(reg->reg_size) +
				+ ", #" + std::to_string(reg->offset.value()) + "]";
		} else {
			return arm_registers[(size_t)reg->reg->name].sizes.at(reg->reg_size);
		}
	} else {
		auto non_reg{std::dynamic_pointer_cast<ASMValNonRegister>(val)};
		if (auto it{var_offsets.find(*non_reg)}; it != var_offsets.end())
			return asm_val_str(std::make_shared<ASMValRegister>(val->held_type, it->second));
		return (non_reg->is_var ? "" : "#") + non_reg->value;
	}
}

std::string ARM64CodeGenerator::basic_translation(const IRCommand& command, const std::string& diff_cmd) {
	std::string ret{(diff_cmd.empty() ? arm_cmds.at(command.type) : diff_cmd) + " "};
	if (auto arg1{command.args.first})
		ret += asm_val_str(arg1.value());
	if (auto arg2{command.args.second})
		ret += ", " + asm_val_str(arg2.value());
	if (auto arg3{command.args.third})
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
	asm_out.push_back("bl " + std::dynamic_pointer_cast<ASMValNonRegister>(command.args.first.value())->value);
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

	bool called_func{(bool)std::stoi(std::dynamic_pointer_cast<ASMValNonRegister>(command.args.second.value())->value)};
	auto sp_sub{std::max(16, ceiling_multiple(-std::stoi(std::dynamic_pointer_cast<ASMValNonRegister>(command.args.first.value())->value), 16))};
	if (called_func) {
		sp_sub += 16;
	}

	sub(IRCommand{IRCommandType::SUB, {
		std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), stack_reg),
		std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), stack_reg),
		std::make_shared<ASMValNonRegister>(create_sz(TypeEnum::U64), std::to_string(sp_sub))
		}});
	sp -= sp_sub;

	if (called_func) {
		sp_sub -= 16;
		set_var(IRCommand{IRCommandType::SET_VAR, {
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), base_reg),
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), get_reg(RegisterName::RetAddress)),
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), 16)
		}});
		add(IRCommand{IRCommandType::ADD, {
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), base_reg),
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), stack_reg),
			std::make_shared<ASMValNonRegister>(create_sz(TypeEnum::U64), std::to_string(sp_sub))
		}});
	}
	bp = sp + 16;
}
void ARM64CodeGenerator::exit_stack(const IRCommand& command) {
	var_offsets.clear();

	ASMValRegister stack_reg{create_sz(TypeEnum::U64), get_reg(RegisterName::Stack)};
	ASMValRegister base_reg{create_sz(TypeEnum::U64), get_reg(RegisterName::Base)};
	auto sp_add{std::max(16, ceiling_multiple(-std::stoi(std::dynamic_pointer_cast<ASMValNonRegister>(command.args.first.value())->value), 16))};

	if ((bool)std::stoi(std::dynamic_pointer_cast<ASMValNonRegister>(command.args.second.value())->value)) {
		sp_add += 16;
		load(IRCommand{IRCommandType::LOAD, {
			std::make_shared<ASMValRegister>(base_reg),
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), get_reg(RegisterName::RetAddress)),
			std::make_shared<ASMValRegister>(create_sz(TypeEnum::U64), 16)
		}});
		bp += 16;
	}
	add(IRCommand{IRCommandType::ADD, {
		std::make_shared<ASMValRegister>(stack_reg),
		std::make_shared<ASMValRegister>(stack_reg),
		std::make_shared<ASMValNonRegister>(create_sz(TypeEnum::U64), std::to_string(sp_add))
	}});
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
void ARM64CodeGenerator::decl_var(const IRCommand& command) {
	IRCommand cmd_copy{command};

	auto max_val_search{var_offsets | std::views::values | std::ranges::to<std::vector>()};
	max_val_search.push_back(-sp);

	std::pair<ASMValNonRegister, int> offset{std::make_pair(
		*cast_non_reg(command.args.first.value()),
		*std::ranges::min_element(max_val_search) - command.args.first.value()->held_type->get_size()
	)};

	var_offsets.insert(offset);
	//std::get<0>(cmd_copy.args) = std::make_shared<ASMValRegister>(command.args.first.value()->held_type, offset.second);

	set_var(cmd_copy);
}

bool ARM64CodeGenerator::is_basic_register(const ASMVal& val) {
	auto reg{cast_reg(val)};
	return (reg != nullptr && !reg->offset.has_value());
}

bool ARM64CodeGenerator::is_register(const ASMVal& val) {
	auto reg{cast_reg(val)};
	return (reg != nullptr);
}

bool ARM64CodeGenerator::is_variable(const ASMVal& val) {
	auto reg{cast_reg(val)};
	return (reg != nullptr && reg->offset.has_value());
}

void ARM64CodeGenerator::set_var(const IRCommand& command) {
	if (is_register(command.args.first.value()) &&
		is_register(command.args.second.value()) &&
		is_register(command.args.third.value())) {
		asm_out.push_back(basic_translation(command, "stp"));
	} else {
		command.args.third.value()->print();
		std::cout << std::endl;
		command.args.first.value()->print();
		std::cout << std::endl;
		std::string str_into{asm_val_str(command.args.first.value())};
		/*move(IRCommand{IRCommandType::MOVE, std::make_tuple(
			command.args.second,
			command.args.third,
			std::nullopt
		)});*/
		//asm_out.push_back("str " + asm_val_str(command.args.second.value()) + ", " + str_into);
		asm_out.push_back("str " + asm_val_str(command.args.third.value()) + ", " + str_into);
	}
}
void ARM64CodeGenerator::load(const IRCommand& command) {
	if (is_register(command.args.first.value()) &&
		is_register(command.args.second.value()) &&
		is_register(command.args.third.value())) {
		asm_out.push_back(basic_translation(command, "ldp"));
	} else {
		auto var{cast_non_reg(command.args.second.value())};
		auto var_reg{std::make_shared<ASMValRegister>(var->held_type, var_offsets[*var])};
		asm_out.push_back("ldr " + asm_val_str(command.args.first.value()) + ", " + asm_val_str(var_reg));
	}
}
void ARM64CodeGenerator::nothing(const IRCommand& command) {

}
void ARM64CodeGenerator::zero(const IRCommand& command) {

}
