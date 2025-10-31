#pragma once

#include <iostream>
#include <memory>
#include <variant>
#include "Lexer.h"

struct TType {
	explicit TType() { }
	virtual ~TType() = default;

	virtual void print(std::ostream& os) const noexcept { }
};

using TType_ptr = std::shared_ptr<TType>;

struct TConstructor : public TType {
	TConstructor() : TType{} {}
	explicit TConstructor(int type_id) : TType{}, type_id{type_id} {}
	explicit TConstructor(int type_id, const std::vector<TType_ptr>& generics)
		: TType{}, type_id{type_id}, generics{generics} {}

	bool operator==(const TConstructor& t) const noexcept {
		return (type_id == t.type_id && generics == t.generics);
	}
	bool operator!=(const TConstructor& t) const noexcept { return !(*this == t); }

	void print(std::ostream& os) const noexcept override { os << "id" << type_id << std::endl; }

	int type_id{};
	std::vector<TType_ptr> generics{};
};

struct TVariable : public TType {
	TVariable() : TType{} {}
	explicit TVariable(int idx) : TType{}, index{idx} {}

	bool operator==(const TVariable& t) const noexcept {
		return (index == t.index);
	}
	bool operator!=(const TVariable& t) const noexcept { return !(*this == t); }

	void print(std::ostream& os) const noexcept override { os << "$" << index << std::endl; }

	int index{};
};

static bool cmp_types(const TType_ptr& t1, const TType_ptr& t2) noexcept;

struct TPointer : public TType {
	TPointer() : TType{} {}
	explicit TPointer(const TType_ptr& t) : TType{}, inner{t} {}

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
	std::string name;
	std::optional<Token> define_token;
	uint8_t size;

	Type() : type_id{-1}, name{""}, define_token{Token{}}, size{0} { }

	Type(int type_id, const Token& define_token, uint8_t size)
		: type_id{type_id}, name{define_token.value}, define_token{define_token}, size{size} { }

	Type(const std::string& name, uint8_t size)
		: type_id{type_id_inc++}, name{name}, define_token{std::nullopt}, size{size} { }

	void print(std::ostream& os) const noexcept {
		if (define_token.has_value()) {
			os << define_token.value();
		} else {
			os << name;
		}
		os << " (" << type_id << ")\n";
	}

	bool operator==(const Type& t) const noexcept {
		return type_id == t.type_id;
	}
};

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

struct MixType {
	MixType() {}
	MixType(const TType_ptr& t) : type{t} {}
	MixType(const Type& t) : type{t} {}

	virtual void print(std::ostream& os = std::cout) {
		std::visit(overloaded {
            [&](const TType_ptr& arg) { arg->print(os); },
            [&](const Type& arg) { arg.print(os); }
        }, type);
    }

	const Type& get_type() const noexcept {
		return std::get<Type>(type);
	}
	const TType_ptr& get_ttype_ptr() const noexcept {
		return std::get<TType_ptr>(type);
	}

	Type& get_type() noexcept {
		return std::get<Type>(type);
	}
	TType_ptr& get_ttype_ptr() noexcept {
		return std::get<TType_ptr>(type);
	}

	template <typename T>
	void operator=(const T& t) noexcept {
		get_type<T>() = type;
	}

	bool operator==(const MixType& t) const noexcept {
		if (std::holds_alternative<TType_ptr>(type) && std::holds_alternative<TType_ptr>(t.type)) {
			return cmp_types(get_ttype_ptr(), t.get_ttype_ptr());
		} else if (std::holds_alternative<Type>(type) && std::holds_alternative<Type>(t.type)) {
			return get_type() == t.get_type();
		} else {
			return false;
		}
	}

private:
	std::variant<TType_ptr, Type> type{};
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

