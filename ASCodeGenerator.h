#pragma once

#include "IntermediateCodeGenerator.h"

struct ASRegister {
	ASRegister(const std::array<std::string, 5>& sizes)
		: sizes{
			std::views::zip(std::array{8, 4, 2, 1, 1}, sizes)
			| std::ranges::to<std::map<uint8_t, std::string>>()
		} {  }

	std::map<uint8_t, std::string> sizes{};
};

static std::vector<ASRegister> as_registers{
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
	{{"rip", "eip", "ip", "", ""}}
};

static std::map<IRCommandType, std::string> as_cmds{
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
	{IRCommandType::LEAVE, "leave"}
};

class ASCodeGenerator {
public:
	ASCodeGenerator(const std::vector<IRCommand>& commands)
		: commands{commands} { }

	const std::vector<std::string>& run();

private:
	std::vector<IRCommand> commands{};
	std::vector<std::string> asm_out{};

	char get_cmd_postfix(uint8_t size);

	std::string asm_cmd(const IRCommand& command, uint8_t cmd_size = 0u);
	std::string asm_val_str(const ASMVal& val);

	std::string basic_translation(const IRCommand& command, uint8_t cmd_size = 0u);

	void preamble();
	void generate_command(const IRCommand& command);
	void move(const IRCommand& command);
	void add(const IRCommand& command);
	void sub(const IRCommand& command);
	void mul(const IRCommand& command);
	void div(const IRCommand& command);
	void xor_cmd(const IRCommand& command);
	void neg(const IRCommand& command);
	void call(const IRCommand& command);
	void ret(const IRCommand& command);
	void func(const IRCommand& command);
	void push(const IRCommand& command);
	void pop(const IRCommand& command);
	void lea(const IRCommand& command);
	void label(const IRCommand& command);
	void directive(const IRCommand& command);
	void leave();
};
