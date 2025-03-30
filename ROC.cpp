#include <sstream>
#include <ostream>
#include "ROC.h"
#include "Parser.h"
#include "TypeAnalyzer.h"
#include "EnvironmentAnalyzer.h"
#include "IntermediateCodeGenerator.h"
#include "ASCodeGenerator.h"

void ROC::run(const std::string& line) {
	Lexer lexer{line};
	auto toks{lexer.run()};

	std::cout << "Lexing completed.\n";

	Parser parser{toks};
	auto stmts{parser.run()};

	std::cout << "Parsing completed.\n";

	TypeAnalyzer ta{stmts};
	if (ta.run()) return;

	EnvironmentAnalyzer ea{stmts};
	if (!ea.run()) return;

	std::cout << "Environment analysis completed.\n";

	IntermediateCodeGenerator icg{stmts};
	auto cmds{icg.run()};

	std::cout << "Intermediate code generation completed.\n";

	ASCodeGenerator as{cmds};
	auto as_cmds{as.run()};

	std::cout << "GAS code generation completed.\n";

	std::ofstream out{"rocout.s"};
	for (auto cmd : as_cmds) {
		out << cmd << '\n';
	}
	out.close();
}

void ROC::run(const std::ifstream& file) {
	std::stringstream ss{};
	ss << file.rdbuf();
	Lexer lexer{ss.str()};
	auto toks{lexer.run()};

	std::cout << "Lexing completed.\n";

	std::ofstream lex_out{"rocout.lex"};
	for (auto tok : toks) {
		lex_out << tok << '\n';
	}

	Parser parser{toks};
	auto stmts{parser.run()};

	std::cout << "Parsing completed.\n";
	
	TypeAnalyzer ta{stmts};
	if (!ta.run()) return;

	std::cout << "Type analysis completed.\n";

	EnvironmentAnalyzer ea{stmts};
	if (!ea.run()) return;

	std::cout << "Environment analysis completed.\n";

	IntermediateCodeGenerator icg{stmts};
	auto cmds{icg.run()};

	std::cout << "Intermediate code generation completed.\n";

	std::ofstream ir_out{"rocout.ir"};
	for (auto cmd : cmds) {
		ir_out << cmd << '\n';
	}

	ASCodeGenerator as{cmds};
	auto as_cmds{as.run()};

	std::cout << "GAS code generation completed.\n";

	std::ofstream out{"rocout.s"};
	for (auto cmd : as_cmds) {
		out << cmd << '\n';
	}
	out.close();
}

