#include "MachineSpecificCodeGenerator.h"
#include <cstdlib>

const std::vector<std::string>& MachineSpecificCodeGenerator::run() {
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

void MachineSpecificCodeGenerator::generate_command(const IRCommand& command) {
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
		case IRCommandType::NEG:
			neg(command);
			return;
		case IRCommandType::XOR:
			xor_cmd(command);
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
		case IRCommandType::ENTER_STACK:
			enter_stack(command);
			return;
		case IRCommandType::EXIT_STACK:
			exit_stack(command);
			return;
		case IRCommandType::LABEL:
			label(command);
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
		case IRCommandType::DIRECTIVE:
			directive(command);
			return;
		case IRCommandType::STORE:
			store(command);
			return;
		case IRCommandType::LOAD:
			load(command);
			return;
		case IRCommandType::NOTHING:
			nothing(command);
			return;
		case IRCommandType::ZERO:
			zero(command);
			return;
		default:
			asm_out.push_back("Not supported just yet ;)");
			return;
	}
}

