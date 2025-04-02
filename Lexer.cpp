#include "Lexer.h"
#include "ErrorHandling.h"

std::vector<Token> Lexer::run() {
	while (!is_at_end()) {
		start = current;
		scan_token();
    }

    tokens.push_back({"", TokenType::END_OF_FILE, line});
    return tokens;
}

void Lexer::scan_token() {
	char c = advance();
	switch (c) {
		case '(': add_token(TokenType::LEFT_PAREN); break;
		case ')': add_token(TokenType::RIGHT_PAREN); break;
		case '{': add_token(TokenType::LEFT_BRACE); break;
		case '}': add_token(TokenType::RIGHT_BRACE); break;
		case ',': add_token(TokenType::COMMA); break;
		case '.': add_token(TokenType::DOT); break;
		case '-':
			if (std::isdigit(peek())) {
				number(true);
			} else {
				add_token(TokenType::MINUS);
			}
			break;
		case '+': add_token(TokenType::PLUS); break;
		case ';': add_token(TokenType::SEMICOLON); break;
		case '*': add_token(TokenType::STAR); break; 
		case '/':
			if (match('/')) {
				while (peek() != '\n' && !is_at_end()) advance();
			} else {
				add_token(TokenType::SLASH);
			}
			break;
		case '!':
			add_token(match('=') ? TokenType::NOT_EQUAL : TokenType::NOT);
			break;
		case '=':
			add_token(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
			break;
		case '<':
			add_token(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
			break;
		case '>':
			add_token(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
			break;
		case '|':
			if (match(c)) {
				add_token(TokenType::OR);
				break;
			}
		case '&':
			add_token(match(c) ? TokenType::AND : TokenType::AMPERSAND);
			break;
		case '\"': string(); break;
		case '\'': char_lit(); break;
		case ' ':
		case '\r':
		case '\t':
			break;
		case '\n':
			line++;
			break;

		default:
			if (std::isdigit(c)) {
				number();
			} else if (std::isalpha(c) || c == '_') {
				identifier();
			} else {
				error(line, "Unknown character."); break;
			}
	}
}

void Lexer::string() {
	while (peek() != '"' && !is_at_end()) {
		if (peek() == '\n') line++;
		advance();
    }

    if (is_at_end()) {
		error(line, "Unterminated string.");
		return;
    }

    advance();

    tokens.push_back({source.substr(start + 1, current-start-2), TokenType::STRING_LITERAL, line});
}

void Lexer::char_lit() {
	while (peek() != '\'' && !is_at_end()) {
		if (peek() == '\n') line++;
		advance();
    }

	if (start - current > 1) {
		error(line, "Character literal can only be one character long.");
	}

    if (is_at_end()) {
		error(line, "Unterminated character literal.");
		return;
    }

    advance();

    tokens.push_back({source.substr(start + 1, current-start-2), TokenType::CHAR_LITERAL, line});
}

void Lexer::number(bool negative) {
	while (std::isdigit(peek())) advance();

	int current{this->current};
	int start{this->start};
	this->start = this->current;

	identifier();

	std::vector<TokenType> types{
		  number_types
		| std::views::values
		| std::views::transform([](const RealType& t) { return t.keyword.second; })
		| std::ranges::to<std::vector>()
	};
	if (std::ranges::find(types, tokens.back().type) == types.end()) {
		this->current = current;
	}

	this->start = start;

	tokens.pop_back();
	add_token(TokenType::NUMBER_LITERAL);
}

void Lexer::identifier() {
	while (std::isalnum(peek()) || peek() == '_') advance();
	std::string text{source.substr(start, current-start)};
    TokenType type{};
	try {
		type = keywords.at(text);
	} catch (const std::exception& e) {
		type = TokenType::IDENTIFIER;
	}
    add_token(type);
}

