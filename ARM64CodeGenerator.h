#pragma once

#include "MachineSpecificCodeGenerator.h"

class ARM64CodeGenerator : public MachineSpecificCodeGenerator {
public:
	ARM64CodeGenerator() : MachineSpecificCodeGenerator{} { }
	ARM64CodeGenerator(const std::vector<IRCommand>& commands)
		: MachineSpecificCodeGenerator{commands} { }

private:
	size_t stack_sub{0u};

	std::string asm_val_str(const ASMVal& val) const override;
	std::string basic_translation(const IRCommand& command);

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
	void leave(const IRCommand& command) override;

	const std::map<IRCommandType, std::string> arm_cmds{
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

	const std::vector<ASMRegister> arm_registers{
		{std::array<std::string, 2>{"x0", "w0"}},
		{std::array<std::string, 2>{"x19", "w19"}},
		{std::array<std::string, 2>{"x4", "w4"}},
		{std::array<std::string, 2>{"x3", "w3"}},
		{std::array<std::string, 2>{"x2", "w2"}},
		{std::array<std::string, 2>{"x1", "w1"}},
		{std::array<std::string, 2>{"x5", "w5"}},
		{std::array<std::string, 2>{"x6", "w6"}},
		{std::array<std::string, 2>{"x9", "w9"}},
		{std::array<std::string, 2>{"x10", "w10"}},
		{std::array<std::string, 2>{"x20", "w20"}},
		{std::array<std::string, 2>{"x21", "w21"}},
		{std::array<std::string, 2>{"x22", "w22"}},
		{std::array<std::string, 2>{"x23", "w23"}},
		{std::array<std::string, 2>{"sp", "sp"}},
		{std::array<std::string, 2>{"x29", "w29"}},
		{std::array<std::string, 2>{"pc", "pc"}}
	};
};

