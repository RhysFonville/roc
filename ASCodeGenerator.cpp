#include "ASCodeGenerator.h"
#include "IntermediateCodeGenerator.h"
#include <memory>

char ASCodeGenerator::get_cmd_postfix(uint8_t size) {
	switch (size) {
		case SZ_R:
			return 'q';
		case SZ_E:
			return 'l';
		case SZ_X:
			return 'w';
		case SZ_H:
			return 'b';
		default:
			return 'x';
	}

	return size;
}

std::string ASCodeGenerator::asm_cmd(const IRCommand& command, uint8_t cmd_size) {
	std::string ret{as_cmds[command.type]};
	if (command.args.first.has_value()) {
		auto get_sz = [](auto val) {
			if (auto lhs{std::dynamic_pointer_cast<ASMValRegister>(val)}) {
				return lhs->held_type->get_size();
			} else {
				return val->held_type->get_size();
			}
		};
		uint8_t size{cmd_size};
		if (cmd_size == 0) {
			size = get_sz(command.args.first.value());
			if (command.args.second.has_value()) {
				size = std::min(size, get_sz(command.args.second.value()));
			}
		}

		ret += get_cmd_postfix(size);
	}
	return ret;
}

std::string ASCodeGenerator::asm_val_str(const ASMVal& val) {
	if (auto reg{std::dynamic_pointer_cast<ASMValRegister>(val)}) {
		std::string ret{};
		if (reg->offset.has_value()) ret += std::to_string(reg->offset.value()) + "(";
		else if (reg->dereferenced) ret += "(";

		ret += "%" + as_registers[(size_t)reg->reg->name].sizes[reg->reg_size]; 

		if (reg->offset.has_value()) ret += ")";
		else if (reg->dereferenced) ret += ")";
		return ret;
	} else {
		return "$" + std::dynamic_pointer_cast<ASMValNonRegister>(val)->value;
	}
}

std::string ASCodeGenerator::basic_translation(const IRCommand& command, uint8_t cmd_size) {
	std::string ret{asm_cmd(command, cmd_size)};
	if (command.args.first.has_value()) {
		ret += " " + asm_val_str(command.args.first.value());
	}
	if (command.args.second.has_value()) {
		ret += ", " + asm_val_str(command.args.second.value());
	}
	return ret;
}

const std::vector<std::string>& ASCodeGenerator::run() {
	for (const IRCommand& command : commands) {
		generate_command(command);
	}
	preamble();

	for (int i{0}; i < asm_out.size(); i++) {
		if (asm_out[i].back() != ':') {
			asm_out[i] = std::string{"\t"} + asm_out[i];
		}
	}
	return asm_out;
}

void ASCodeGenerator::preamble() {
	asm_out.insert(asm_out.begin(), ".text");
}

void ASCodeGenerator::generate_command(const IRCommand& command) {
	switch (command.type) {
		case IRCommandType::MOVE:
			move(command);
			return;
		case IRCommandType::ADD:
			add(command);
			return;
		case IRCommandType::SUB:
			sub(command);
			return;
		case IRCommandType::MULT:
			mul(command);
			return;
		case IRCommandType::DIV:
			div(command);
			return;
		case IRCommandType::XOR:
			xor_cmd(command);
			return;
		case IRCommandType::NEG:
			neg(command);
			return;
		case IRCommandType::CALL:
			call(command);
			return;
		case IRCommandType::RET:
			ret(command);
			return;
		case IRCommandType::FUNC:
			func(command);
			return;
		case IRCommandType::PUSH:
			push(command);
			return;
		case IRCommandType::POP:
			pop(command);
			return;
		case IRCommandType::LEA:
			lea(command);
			return;
		case IRCommandType::LABEL:
			label(command);
			return;
		case IRCommandType::DIRECTIVE:
			directive(command);
			return;
		case IRCommandType::LEAVE:
			leave();
			return;
		default:
			asm_out.push_back("Not supported just yet ;)");
			return;
	}
}

void ASCodeGenerator::move(const IRCommand& command) {
	auto reg_lhs{std::dynamic_pointer_cast<ASMValRegister>(command.args.first.value())};
	auto reg_rhs{std::dynamic_pointer_cast<ASMValRegister>(command.args.second.value())};
	if (reg_lhs != nullptr && reg_rhs != nullptr) {
		// If mem -> mem
		if ((reg_lhs->reg->name == RegisterName::Base || reg_lhs->dereferenced) && (reg_rhs->reg->name == RegisterName::Base || reg_rhs->dereferenced)) {
			auto reg{std::make_shared<ASMValRegister>(reg_lhs->held_type, occupy_next_reg())};
			IRCommand temp_move{IRCommandType::MOVE, std::make_pair(reg_lhs, reg)};
			IRCommand move_into{IRCommandType::MOVE, std::make_pair(reg, command.args.second)};
			move(temp_move);
			move(move_into);
			return;
		}
	}

	auto get_side_sz = [&](const auto& side) {
		uint8_t sz{side->held_type->get_size()};
		if (auto reg{std::dynamic_pointer_cast<ASMValRegister>(side)}) {
			if (!reg->offset.has_value())
				sz = reg->reg_size;
		}
		return sz;
	};
	uint8_t lhs_size{get_side_sz(command.args.first.value())};
	uint8_t rhs_size{get_side_sz(command.args.second.value())};

	if (lhs_size < rhs_size) {
		bool signed_rhs{command.args.second.value()->held_type->is_signed()};
		std::string postfix{signed_rhs ? "s" : "z"};
		postfix += get_cmd_postfix(lhs_size);
		postfix += get_cmd_postfix(rhs_size);

		asm_out.push_back(as_cmds[command.type] + postfix + " " +
			asm_val_str(command.args.first.value()) + ", " +
			asm_val_str(command.args.second.value())
		);
	} else {
		asm_out.push_back(as_cmds[command.type] + get_cmd_postfix(rhs_size) + " " +
			asm_val_str(command.args.first.value()) + ", " +
			asm_val_str(command.args.second.value())
		);
	}
}

void ASCodeGenerator::add(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ASCodeGenerator::sub(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ASCodeGenerator::mul(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ASCodeGenerator::div(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ASCodeGenerator::xor_cmd(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ASCodeGenerator::neg(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ASCodeGenerator::call(const IRCommand& command) {
	asm_out.push_back(as_cmds[command.type] + " " + std::dynamic_pointer_cast<ASMValNonRegister>(command.args.first.value())->value);
}

void ASCodeGenerator::ret(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ASCodeGenerator::func(const IRCommand& command) {
	asm_out.push_back(".global " + std::dynamic_pointer_cast<ASMValNonRegister>(command.args.first.value())->value);
	asm_out.push_back(std::dynamic_pointer_cast<ASMValNonRegister>(command.args.first.value())->value + ":");
}

void ASCodeGenerator::push(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ASCodeGenerator::pop(const IRCommand& command) {
	asm_out.push_back(basic_translation(command));
}

void ASCodeGenerator::lea(const IRCommand& command) {
	asm_out.push_back(basic_translation(command, SZ_R));
}

void ASCodeGenerator::label(const IRCommand& command) {
	asm_out.push_back(std::dynamic_pointer_cast<ASMValNonRegister>(command.args.first.value())->value + ":");
}

void ASCodeGenerator::directive(const IRCommand& command) {
	asm_out.push_back(
		"." +
		std::dynamic_pointer_cast<ASMValNonRegister>(command.args.first.value())->value +
		" " +
		std::dynamic_pointer_cast<ASMValNonRegister>(command.args.second.value())->value
	);
}

void ASCodeGenerator::leave() {
	asm_out.push_back("leave");
}

