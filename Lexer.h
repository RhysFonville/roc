#pragma once

#include <string>
#include <vector>
#include <ostream>
#include <map>
#include <ranges>

enum class TokenType {
	// Standard operators.
	LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
	COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,
	AMPERSAND,

	// Bool operators.
	NOT, NOT_EQUAL,
	EQUAL, EQUAL_EQUAL,
	GREATER, GREATER_EQUAL,
	LESS, LESS_EQUAL,
	OR, AND,

	// Literals.
	IDENTIFIER, STRING_LITERAL, NUMBER_LITERAL, CHAR_LITERAL,
	TRUE, FALSE,

	// Primitive type keywords.
	U8, U16, U32, U64, I8, I16,
	I32, I64, BOOL, NONE, AUTO,

	// Keywords.
	ELSE, FOR, IF, AS,
	RETURN, WHILE,

	// Misc.
	END_OF_FILE
};

static const std::map<std::string, TokenType> keywords{
	{"i8",    TokenType::I8},
	{"i16",    TokenType::I16},
	{"i32",    TokenType::I32},
	{"i64",    TokenType::I64},
	{"u8",    TokenType::U8},
	{"u16",    TokenType::U16},
	{"u32",    TokenType::U32},
	{"u64",    TokenType::U64},
	{"bool",   TokenType::BOOL},
	{"none",   TokenType::NONE},
	{"auto",   TokenType::AUTO},
	{"if",     TokenType::IF},
	{"else",   TokenType::ELSE},
	{"return", TokenType::RETURN},
	{"for",    TokenType::FOR},
	{"while",  TokenType::WHILE},
	{"true",   TokenType::TRUE},
	{"false",  TokenType::FALSE},
	{"as",  TokenType::AS}
};

static const std::vector<TokenType> literal_tokens{
	TokenType::IDENTIFIER, TokenType::STRING_LITERAL,
	TokenType::NUMBER_LITERAL, TokenType::CHAR_LITERAL,
	TokenType::TRUE, TokenType::FALSE
};

struct Token {
	std::string value{};
	TokenType type{};
	unsigned int line{};

	bool operator<(const Token& tok) const noexcept { return value.compare(tok.value) < 0; }
	friend inline std::ostream& operator<<(std::ostream& os, const Token& tok);
};

inline std::ostream& operator<<(std::ostream& os, const Token& tok) {
	os << std::to_string((int)tok.type) + ": " + tok.value + " (line " + std::to_string(tok.line) + ")";
	return os;
}

class Lexer {
public:
	Lexer() { }
	Lexer(const std::string& source) : source{source} { }
	std::vector<Token> run();

private:
	std::string source{};

	int start{};
	int current{};
	unsigned int line{1u};

	std::vector<Token> tokens{};

	bool is_at_end() { return current >= source.size(); }

	char advance() {
		return source[current++];
	}
	char peek() {
		if (is_at_end()) return '\0';
		return source[current];
	}
	void add_token(TokenType type) {
		tokens.push_back({source.substr(start, current-start), type, line});
	}

	bool match(char expected) {
		if (is_at_end()) return false;
		if (source[current] != expected) return false;
		
		current++;
		return true;
	}

	void scan_token();

	void string();
	void char_lit();
	void number(bool negative = false);
	void identifier();
};

