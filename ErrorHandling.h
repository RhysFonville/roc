#pragma once

#include <string>
#include <iostream>
#include "Lexer.h"

static void report(unsigned int line, const std::string& where, const std::string& message) {
	std::cerr << "Line " << std::to_string(line) << ' ' << where << ": " << message << std::endl;
}

static void error(unsigned int line, const std::string& message) noexcept {
	std::cout << "\033[1;31m";
	report(line, "", message);
	std::cout << "\033[0m";
}

static void error(const Token& token, const std::string& message) {
	std::cout << "\033[1;31m";
	if (token.type == TokenType::END_OF_FILE) {
		report(token.line, "at end", message);
	} else {
		report(token.line, "at '" + token.value + "'", message);
	}
	std::cout << "\033[0m";
}

