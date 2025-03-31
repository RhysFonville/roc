#pragma once

#include <string>
#include <variant>
#include <vector>
#include <ostream>
#include <map>
#include <ranges>
#include <memory>

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

	// Type keywords.
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

enum class TypeEnum {
	I8, I16, I32, I64, U8, U16, U32, U64, BOOL, NONE
};

class RealType {
public:
	RealType() { }
	RealType(std::pair<std::string, TokenType> keyword, uint8_t size)
		: keyword{keyword}, size{size} { }

	bool operator==(const RealType& type) const {
		return keyword == type.keyword && size == type.size && is_signed == type.is_signed;
	}
	bool operator!=(const RealType& type) const { return !(*this == type); }
	friend inline std::ostream& operator<<(std::ostream& os, const RealType& type);

	std::pair<std::string, TokenType> keyword{};
	uint8_t size{};
	bool is_signed{};
};

inline std::ostream& operator<<(std::ostream& os, const RealType& type) {
	os << type.keyword.first;
	return os;
}

static const std::map<TypeEnum, RealType> types{
	{TypeEnum::I8, {*keywords.find("i8"), sizeof(int8_t)}},
	{TypeEnum::I16, {*keywords.find("i16"), sizeof(int16_t)}},
	{TypeEnum::I32, {*keywords.find("i32"), sizeof(int32_t)}},
	{TypeEnum::I64, {*keywords.find("i64"), sizeof(int64_t)}},
	{TypeEnum::U8, {*keywords.find("u8"), sizeof(uint8_t)}},
	{TypeEnum::U16, {*keywords.find("u16"), sizeof(uint16_t)}},
	{TypeEnum::U32, {*keywords.find("u32"), sizeof(uint32_t)}},
	{TypeEnum::U64, {*keywords.find("u64"), sizeof(uint64_t)}},
	{TypeEnum::BOOL, {*keywords.find("bool"), sizeof(int8_t)}},
	{TypeEnum::NONE, {*keywords.find("none"), 0u}}
};

static const std::map<TypeEnum, RealType> number_types{
	*types.find(TypeEnum::I8), *types.find(TypeEnum::I16),
	*types.find(TypeEnum::I32), *types.find(TypeEnum::I64),
	*types.find(TypeEnum::U8), *types.find(TypeEnum::U16),
	*types.find(TypeEnum::U32), *types.find(TypeEnum::U64)
};

static std::vector<TokenType> type_tokens() {
	std::vector<TokenType> t{types | std::views::values | std::views::transform([](const auto& type) {
		return type.keyword.second;
	}) | std::ranges::to<std::vector>()};
	t.insert(t.end(), TokenType::AUTO);
	return t;
}

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

static std::optional<RealType> token_to_type(const Token& token) {
	for (const RealType& type : types | std::views::values) {
		if (type.keyword.second == token.type) return type;
	}
	return std::nullopt;
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

