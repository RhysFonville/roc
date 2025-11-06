#pragma once

#include <iostream>
#include <memory>
#include <variant>
#include <concepts>
#include <ranges>
#include "Lexer.h"

struct ParseType {
	ParseType() { }
	ParseType(const Token& token) { }
	ParseType(const std::shared_ptr<ParseType>& t) { }

	bool operator==(const ParseType& t) const noexcept {
		if (type.has_value() && t.type.has_value()) {
			if (std::holds_alternative<Token>(type.value()) &&
				std::holds_alternative<Token>(t.type.value())) {
				return std::get<Token>(type.value()).value ==
					std::get<Token>(t.type.value()).value;
			} else if (std::holds_alternative<std::shared_ptr<ParseType>>(type.value()) &&
					std::holds_alternative<std::shared_ptr<ParseType>>(t.type.value())) {
				return *std::get<std::shared_ptr<ParseType>>(type.value()) ==
					*std::get<std::shared_ptr<ParseType>>(t.type.value());
			}
		}
		return false;
	}

	void print(std::ostream& os) const noexcept {
		if (type.has_value()) {
			if (std::holds_alternative<Token>(type.value())) {
				os << std::get<Token>(type.value());
			} else {
				std::get<std::shared_ptr<ParseType>>(type.value())->print(os);
				os << '*';
			}
		} else {
			os << "Unknown type";
		}
	}
	
	std::optional<std::variant<Token, std::shared_ptr<ParseType>>> type;
};

struct TType {
	explicit TType() { }
	virtual ~TType() = default;

	virtual void print(std::ostream& os) const noexcept { }
	virtual void println(std::ostream& os) const noexcept {
		print(os);
		os << std::endl;
	}
};

using TType_ptr = std::shared_ptr<TType>;

struct TConstructor : public TType {
	TConstructor() {}
	explicit TConstructor(int type_id)
		: type_id{type_id} {}
	explicit TConstructor(int type_id, const std::vector<TType_ptr>& generics)
		: type_id{type_id}, generics{generics} {}

	bool operator==(const TConstructor& t) const noexcept {
		return (type_id == t.type_id && generics == t.generics);
	}
	bool operator!=(const TConstructor& t) const noexcept { return !(*this == t); }

	void print(std::ostream& os) const noexcept override { os << "id" << type_id << std::endl; }

	int type_id{};
	std::vector<TType_ptr> generics{};
};

struct TVariable : public TType {
	TVariable() {}
	explicit TVariable(int idx) : index{idx} {}

	bool operator==(const TVariable& t) const noexcept {
		return (index == t.index);
	}
	bool operator!=(const TVariable& t) const noexcept { return !(*this == t); }

	void print(std::ostream& os) const noexcept override { os << "$" << index << std::endl; }

	int index{};
};

static bool cmp_types(const TType_ptr& t1, const TType_ptr& t2) noexcept;

struct TPointer : public TType {
	TPointer() {}
	explicit TPointer(const TType_ptr& t) : inner{t} {}

	bool operator==(const TPointer& t) const noexcept {
		return cmp_types(inner, t.inner);
	}

	void print(std::ostream& os) const noexcept override {
		os << "pointer to ";
		inner->print(os);
	}

	TType_ptr inner{};
};

static bool is_pointer(const TType_ptr& t) {
	return std::dynamic_pointer_cast<TPointer>(t) != nullptr;
}

static bool cmp_types(const TType_ptr& t1, const TType_ptr& t2) noexcept {
	auto c1{std::dynamic_pointer_cast<TConstructor>(t1)};
	auto c2{std::dynamic_pointer_cast<TConstructor>(t2)};
	auto v1{std::dynamic_pointer_cast<TVariable>(t1)};
	auto v2{std::dynamic_pointer_cast<TVariable>(t2)};
	auto p1{std::dynamic_pointer_cast<TPointer>(t1)};
	auto p2{std::dynamic_pointer_cast<TPointer>(t2)};

	if (c1 != nullptr && c2 != nullptr) {
		return (*c1 == *c2);
	} else if (v1 != nullptr && v2 != nullptr) {
		return (*v1 == *v2);
	} else if (p1 != nullptr && p2 != nullptr) {
		return (*p1 == *p2);
	}

	return false;
}

static int type_id_inc{0};

struct Type {
	int type_id;
	uint8_t size;
	std::shared_ptr<Type> point_to;

	Type() : type_id{-1}, name{""}, size{0} { }

	Type(int type_id, const Token& define_token, uint8_t size)
		: type_id{type_id}, name{define_token.value}, size{size} { }

	void print(std::ostream& os) const noexcept {
		os << name << " (" << type_id << ")\n";
	}

	bool operator==(const Type& t) const noexcept {
		return type_id == t.type_id;
	}
};

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

struct MixType {
	MixType() {}
	MixType(const ParseType& t) : type{t} {}
	MixType(const TType_ptr& t) : type{t} {}
	MixType(const Type& t) : type{t} {}

	virtual void print(std::ostream& os = std::cout) const noexcept {
		std::visit(overloaded {
            [&](const TType_ptr& arg) { arg->print(os); },
            [&](const auto& arg) { arg.print(os); }
        }, type);
    }

	const ParseType& get_parse_type() const noexcept {
		return std::get<ParseType>(type);
	}
	const Type& get_type() const noexcept {
		return std::get<Type>(type);
	}
	const TType_ptr& get_ttype_ptr() const noexcept {
		return std::get<TType_ptr>(type);
	}

	ParseType& get_parse_type() noexcept {
		return std::get<ParseType>(type);
	}
	Type& get_type() noexcept {
		return std::get<Type>(type);
	}
	TType_ptr& get_ttype_ptr() noexcept {
		return std::get<TType_ptr>(type);
	}

	template <typename T> requires
		std::same_as<T, ParseType> or
		std::same_as<T, TType_ptr> or
		std::same_as<T, Type>
	void operator=(const T& t) noexcept {
		get_type<T>() = type;
	}

	bool operator==(const MixType& t) const noexcept {
		if (std::holds_alternative<ParseType>(type) && std::holds_alternative<ParseType>(t.type)) {
			return get_parse_type() == t.get_parse_type();
		} else if (std::holds_alternative<TType_ptr>(type) && std::holds_alternative<TType_ptr>(t.type)) {
			return cmp_types(get_ttype_ptr(), t.get_ttype_ptr());
		} else if (std::holds_alternative<Type>(type) && std::holds_alternative<Type>(t.type)) {
			return get_type() == t.get_type();
		} else {
			return false;
		}
	}

	std::optional<int> get_type_index() const noexcept {
		if (std::holds_alternative<TType_ptr>(type)) {
			if (auto con{std::dynamic_pointer_cast<TConstructor>(get_ttype_ptr())}) {
				return con->type_id;
			}
		} else if (std::holds_alternative<Type>(type)) {
			return get_type().type_id;
		}
		return std::nullopt;
	}

private:
	std::variant<ParseType, TType_ptr, Type> type{};
	std::optional<Token> defined_at{}; // Primitives have no defined token
	std::string name{};
};

enum class PrimitiveTypeEnum {
	I8, I16, I32, I64, U8, U16, U32, U64, BOOL, NONE
};

class PrimitiveType : public Type {
public:
	PrimitiveType() {}
	PrimitiveType(Type type, bool is_signed) : Type{type}, is_signed{is_signed} {}

	bool operator==(const PrimitiveType& type) const {
		return (Type::operator==(type) && is_signed == type.is_signed);
	}
	bool operator!=(const PrimitiveType& type) const { return !(*this == type); }
	friend inline std::ostream& operator<<(std::ostream& os, const PrimitiveType& type);

	bool is_signed{};
	TokenType token_type{};
};

static const std::map<PrimitiveTypeEnum, PrimitiveType> primitive_types{
	{PrimitiveTypeEnum::I8,		PrimitiveType{Type{"i8",	sizeof(int8_t)},	true}},
	{PrimitiveTypeEnum::I16,	PrimitiveType{Type{"i16",	sizeof(int16_t)},	true}},
	{PrimitiveTypeEnum::I32,	PrimitiveType{Type{"i32",	sizeof(int32_t)},	true}},
	{PrimitiveTypeEnum::I64,	PrimitiveType{Type{"i64",	sizeof(int64_t)},	true}},
	{PrimitiveTypeEnum::U8,		PrimitiveType{Type{"u8",	sizeof(uint8_t)},	true}},
	{PrimitiveTypeEnum::U16,	PrimitiveType{Type{"u16",	sizeof(uint16_t)},	true}},
	{PrimitiveTypeEnum::U32,	PrimitiveType{Type{"u32",	sizeof(uint32_t)},	true}},
	{PrimitiveTypeEnum::U64,	PrimitiveType{Type{"u64",	sizeof(uint64_t)},	true}},
	{PrimitiveTypeEnum::BOOL,	PrimitiveType{Type{"bool",	sizeof(int8_t)},	true}},
	{PrimitiveTypeEnum::NONE,	PrimitiveType{Type{"none",	sizeof(int8_t)},	true}},
};

static const std::map<PrimitiveTypeEnum, PrimitiveType> number_types{
	*primitive_types.find(PrimitiveTypeEnum::I8), *primitive_types.find(PrimitiveTypeEnum::I16),
	*primitive_types.find(PrimitiveTypeEnum::I32), *primitive_types.find(PrimitiveTypeEnum::I64),
	*primitive_types.find(PrimitiveTypeEnum::U8), *primitive_types.find(PrimitiveTypeEnum::U16),
	*primitive_types.find(PrimitiveTypeEnum::U32), *primitive_types.find(PrimitiveTypeEnum::U64)
};

static std::vector<TokenType> primitive_type_tokens() {
	std::vector<TokenType> t{primitive_types | std::views::values | std::views::transform([](const auto& type) {
		return type.token_type;
	}) | std::ranges::to<std::vector>()};
	t.insert(t.end(), TokenType::AUTO);
	return t;
}

static std::optional<PrimitiveType> token_to_primitive_type(const Token& token) {
	for (const PrimitiveType& type : primitive_types | std::views::values) {
		if (type.token_type == token.type) return type;
	}
	return std::nullopt;
}

