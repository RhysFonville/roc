#pragma once

#include "MachineSpecificCodeGenerator.h"

class ASCodeGenerator : public MachineSpecificCodeGenerator {
public:
	ASCodeGenerator() : MachineSpecificCodeGenerator{} { }
	ASCodeGenerator(const std::vector<IRCommand>& commands)
		: MachineSpecificCodeGenerator{commands} { }

private:
	char get_cmd_postfix(uint8_t size);

	std::string asm_cmd(const IRCommand& command, const std::optional<ASMVal>& arg1, const std::optional<ASMVal>& arg2 = std::nullopt, uint8_t cmd_size = 0u);
	std::string asm_val_str(const ASMVal& val) const override;

	std::string basic_translation(const IRCommand& command, uint8_t cmd_size = 0u);

	void preamble() override;
	void move(const IRCommand& command) override;
	void add(const IRCommand& command) override;
	void sub(const IRCommand& command) override;
	void mul(const IRCommand& command) override;
	void div(const IRCommand& command) override;
	void xor_cmd(const IRCommand& command) override;
	void neg(const IRCommand& command) override;
	void call(const IRCommand& command) override;
	void ret(const IRCommand& command) override;
	void func(const IRCommand& command) override;
	void push(const IRCommand& command) override;
	void pop(const IRCommand& command) override;
	void lea(const IRCommand& command) override;
	void label(const IRCommand& command) override;
	void directive(const IRCommand& command) override;
	void exit_stack(const IRCommand& command) override;

	const std::map<IRCommandType, std::string> as_cmds{
		{IRCommandType::MOVE, "mov"},
		{IRCommandType::ADD, "add"},
		{IRCommandType::SUB, "sub"},
		{IRCommandType::MULT, "mul"},
		{IRCommandType::DIV, "div"},
		{IRCommandType::XOR, "xor"},
		{IRCommandType::NEG, "neg"},
		{IRCommandType::CALL, "call"},
		{IRCommandType::RET, "ret"},
		{IRCommandType::PUSH, "push"},
		{IRCommandType::POP, "pop"},
		{IRCommandType::LEA, "lea"},
		{IRCommandType::EXIT_STACK, "leave"}
	};

	const std::vector<ASMRegister> as_registers{
		{{"rax", "eax", "ax", "al", "ah"}},
		{{"rbx", "ebx", "bx", "bl", "bh"}},
		{{"rcx", "ecx", "cx", "cl", "ch"}},
		{{"rdx", "edx", "dx", "dl", "dh"}},
		{{"rsi", "esi", "si", "sil", ""}},
		{{"rdi", "edi", "di", "dil", ""}},
		{{"r8", "r8d", "r8w", "r8b", ""}},
		{{"r9", "r9d", "r9w", "r9b", ""}},
		{{"r10", "r10d", "r10w", "r10b", ""}},
		{{"r11", "r11d", "r11w", "r11b", ""}},
		{{"r12", "r12d", "r12w", "r12b", ""}},
		{{"r13", "r13d", "r13w", "r13b", ""}},
		{{"r14", "r14d", "r14w", "r14b", ""}},
		{{"r15", "r15d", "r15w", "r15b", ""}},
		{{"rsp", "esp", "sp", "spl", ""}},
		{{"rbp", "ebp", "bp", "bpl", ""}},
		{{"rip", "eip", "ip", "", ""}},
		{{"", "", "", "", ""}}
	};
};
