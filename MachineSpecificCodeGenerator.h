#pragma once

#include "IntermediateCodeGenerator.h"

struct ASMRegister {
	ASMRegister(const std::array<std::string, 5>& sizes)
		: sizes{
			std::views::zip(std::array{SZ_R, SZ_E, SZ_W, SZ_H, SZ_L}, sizes)
			| std::ranges::to<std::map<uint8_t, std::string>>()
		} {  }
	/*ASMRegister(const std::array<std::string, 2>& sizes)
		: ASMRegister{std::views::concat(
			sizes | std::views::take(2), std::views::repeat(sizes[1]) | std::views::take(3)
		)} { }*/
	ASMRegister(const std::array<std::string, 2>& sizes)
		: ASMRegister{{sizes[0], sizes[1], sizes[1], sizes[1], sizes[1]}} { }

	std::map<uint8_t, std::string> sizes{};
};

class MachineSpecificCodeGenerator {
public:
	MachineSpecificCodeGenerator() { }
	MachineSpecificCodeGenerator(const std::vector<IRCommand>& commands)
		: commands{commands} { }

	virtual const std::vector<std::string>& run();

protected:
	std::vector<IRCommand> commands{};
	std::vector<std::string> asm_out{};

	static const std::optional<ASMVal>& get_first(const IRCommand& cmd) noexcept { return std::get<0>(cmd.args); }
	static const std::optional<ASMVal>& get_second(const IRCommand& cmd) noexcept { return std::get<1>(cmd.args); }
	static const std::optional<ASMVal>& get_third(const IRCommand& cmd) noexcept { return std::get<2>(cmd.args); }

	virtual std::string asm_val_str(const ASMVal& val) const { return ""; }

	virtual void generate_command(const IRCommand& command);

	virtual void preamble() { }
	virtual void move(const IRCommand& command) { }
	virtual void add(const IRCommand& command) { }
	virtual void sub(const IRCommand& command) { }
	virtual void mul(const IRCommand& command) { }
	virtual void div(const IRCommand& command) { }
	virtual void xor_cmd(const IRCommand& command) { }
	virtual void neg(const IRCommand& command) { }
	virtual void call(const IRCommand& command) { }
	virtual void ret(const IRCommand& command) { }
	virtual void func(const IRCommand& command) { }
	virtual void push(const IRCommand& command) { }
	virtual void pop(const IRCommand& command) { }
	virtual void lea(const IRCommand& command) { }
	virtual void label(const IRCommand& command) { }
	virtual void directive(const IRCommand& command) { }
	virtual void leave(const IRCommand& command) { }
};

