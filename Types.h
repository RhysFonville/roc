#pragma once

#include <iostream>
#include <memory>
#include <variant>
#include <concepts>
#include <ranges>
#include "Lexer.h"

struct ParseType {
	ParseType() : type{std::nullopt} { }
	ParseType(const Token& token) : type{token} { }
	ParseType(const std::shared_ptr<ParseType>& t) : type{t} { }

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

	std::optional<Token> get_base_type_token() const noexcept {
		if (type.has_value()) {
			const ParseType* ptr{this};
			while (std::holds_alternative<std::shared_ptr<ParseType>>(ptr->type.value())) {
				ptr = std::get<std::shared_ptr<ParseType>>(ptr->type.value()).get();
			}

			return std::get<Token>(ptr->type.value());
		}

		return std::nullopt;
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
	void println(std::ostream& os = std::cout) const noexcept {
		print(os);
		os << std::endl;
	}
	
	std::optional<int> type_id;
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

	Type() : type_id{-1}, size{0} { }

	Type(int type_id)
		: type_id{type_id}, size{0} { }
	
	Type(const std::shared_ptr<Type>& point)
		: point_to{point}, type_id{point->type_id}, size{sizeof(void*)} {}

	Type(int type_id, uint8_t size) : type_id{type_id}, size{size} { }

	void print(std::ostream& os) const noexcept {
		os << type_id << " (sz " << std::to_string(size) << ")";
	}
	void println(std::ostream& os = std::cout) const noexcept {
		print(os);
		os << std::endl;
	}

	bool operator==(const Type& t) const noexcept {
		return type_id == t.type_id;
	}

	bool operator<(const Type& t) const noexcept {
		return type_id < t.type_id;
	}
};

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

struct MixType {
	MixType() {}
	MixType(const ParseType& pt, const Type& t) : parse_type{pt}, type{t} {}
	MixType(const ParseType& t) : parse_type{t} {}

	virtual void print(std::ostream& os = std::cout) const noexcept {
		if (type.has_value()) {
			type.value().print(os);
		} else if (ttype_ptr.has_value()) {
			ttype_ptr.value()->print(os);
		} else if (parse_type.has_value()) {
			parse_type.value().print(os);
		}
    }

	void println(std::ostream& os = std::cout) const noexcept {
		print(os);
		os << std::endl;
	}

	void print_full(std::ostream& os = std::cout) const noexcept {
		if (type.has_value()) {
			type.value().print(os);
		}
		if (ttype_ptr.has_value()) {
			ttype_ptr.value()->print(os);
		}
		if (parse_type.has_value()) {
			parse_type.value().print(os);
		}
	}

	void println_full(std::ostream& os = std::cout) const noexcept {
		if (type.has_value()) {
			type.value().println(os);
		}
		if (ttype_ptr.has_value()) {
			ttype_ptr.value()->println(os);
		}
		if (parse_type.has_value()) {
			parse_type.value().println(os);
		}
		os << std::endl;
	}

	const ParseType& get_parse_type() const noexcept {
		return parse_type.value();
	}
	const Type& get_type() const noexcept {
		return type.value();
	}
	const TType_ptr& get_ttype_ptr() const noexcept {
		return ttype_ptr.value();
	}

	ParseType& get_parse_type() noexcept {
		return parse_type.value();
	}
	TType_ptr& get_ttype_ptr() noexcept {
		return ttype_ptr.value();
	}
	Type& get_type() noexcept {
		return type.value();
	}

	std::optional<ParseType>& get_parse_type_opt() noexcept {
		return parse_type;
	}
	std::optional<TType_ptr>& get_ttype_ptr_opt() noexcept {
		return ttype_ptr;
	}
	std::optional<Type>& get_type_opt() noexcept {
		return type;
	}

	void operator=(const ParseType& t) noexcept {
		get_parse_type_opt() = t;
	}
	void operator=(const TType_ptr& t) noexcept {
		get_ttype_ptr_opt() = t;
	}
	void operator=(const Type& t) noexcept {
		get_type_opt() = t;
	}

	bool operator==(const ParseType& t) noexcept {
		return get_parse_type() == t;
	}
	bool operator==(const TType_ptr& t) noexcept {
		return get_ttype_ptr() == t;
	}
	bool operator==(const Type& t) noexcept {
		return get_type() == t;
	}
	bool operator==(const MixType& t) const noexcept {
		if (type.has_value() && t.type.has_value()) {
			return type == t.type;
		} else if (ttype_ptr.has_value() && t.ttype_ptr.has_value()) {
			return ttype_ptr == t.ttype_ptr;
		} else if (parse_type.has_value() && t.parse_type.has_value()) {
			return parse_type == t.parse_type;
		}
		return false;
	}

	std::optional<int> get_type_index() const noexcept {
		if (type.has_value()) {
			return type.value().type_id;
		} else if (ttype_ptr.has_value()) {
			TType_ptr current_type{ttype_ptr.value()};
			while (auto ptr{std::dynamic_pointer_cast<TPointer>(current_type)}) {
				current_type = ptr->inner;
			}

			if (auto con{std::dynamic_pointer_cast<TConstructor>(current_type)}) {
				return con->type_id;
			}
		} else if (parse_type.has_value()) {
			if (parse_type.value().type_id.has_value()) {
				return parse_type.value().type_id.value();
			}
		}
		return std::nullopt;
	}

	bool is_pointer() const noexcept {
		if (type.has_value()) {
			return (type.value().point_to != nullptr);
		} else if (ttype_ptr.has_value()) {
			return (std::dynamic_pointer_cast<TPointer>(ttype_ptr.value()) != nullptr);
		} else {
			return false;
		}
	}

private:
	std::optional<ParseType> parse_type{};
	std::optional<TType_ptr> ttype_ptr{};
	std::optional<Type> type{};
};

enum class PrimitiveTypeEnum {
	I8, I16, I32, I64, U8, U16, U32, U64, BOOL, NONE
};

class PrimitiveType : public Type {
public:
	PrimitiveType() {}
	PrimitiveType(const std::string& name, Type type, bool is_signed)
		: Type{type}, name{name}, is_signed{is_signed} {}

	bool operator==(const PrimitiveType& type) const {
		return (Type::operator==(type) && is_signed == type.is_signed);
	}
	bool operator!=(const PrimitiveType& type) const { return !(*this == type); }
	friend inline std::ostream& operator<<(std::ostream& os, const PrimitiveType& type);

	MixType to_mix_type() const noexcept {
		return MixType{ParseType{Token{name, TokenType::IDENTIFIER, 0}}, (Type)*this};
	}

	bool is_signed{};
	std::string name{};
};

static const std::map<PrimitiveTypeEnum, PrimitiveType> primitive_types{
	{PrimitiveTypeEnum::I8,		PrimitiveType{"i8",		Type{type_id_inc++, (uint8_t)sizeof(int8_t)},		true}},
	{PrimitiveTypeEnum::I16,	PrimitiveType{"i16",	Type{type_id_inc++, (uint8_t)sizeof(int16_t)},		true}},
	{PrimitiveTypeEnum::I32,	PrimitiveType{"i32",	Type{type_id_inc++, (uint8_t)sizeof(int32_t)},		true}},
	{PrimitiveTypeEnum::I64,	PrimitiveType{"i64",	Type{type_id_inc++, (uint8_t)sizeof(int64_t)},		true}},
	{PrimitiveTypeEnum::U8,		PrimitiveType{"u8",		Type{type_id_inc++, (uint8_t)sizeof(uint8_t)},		true}},
	{PrimitiveTypeEnum::U16,	PrimitiveType{"u16",	Type{type_id_inc++, (uint8_t)sizeof(uint16_t)},	true}},
	{PrimitiveTypeEnum::U32,	PrimitiveType{"u32",	Type{type_id_inc++, (uint8_t)sizeof(uint32_t)},	true}},
	{PrimitiveTypeEnum::U64,	PrimitiveType{"u64",	Type{type_id_inc++, (uint8_t)sizeof(uint64_t)},	true}},
	{PrimitiveTypeEnum::BOOL,	PrimitiveType{"bool",	Type{type_id_inc++, (uint8_t)sizeof(int8_t)},		true}},
	{PrimitiveTypeEnum::NONE,	PrimitiveType{"none",	Type{type_id_inc++, (uint8_t)sizeof(int8_t)},		true}},
};

static const std::map<PrimitiveTypeEnum, PrimitiveType> number_types{
	*primitive_types.find(PrimitiveTypeEnum::I8), *primitive_types.find(PrimitiveTypeEnum::I16),
	*primitive_types.find(PrimitiveTypeEnum::I32), *primitive_types.find(PrimitiveTypeEnum::I64),
	*primitive_types.find(PrimitiveTypeEnum::U8), *primitive_types.find(PrimitiveTypeEnum::U16),
	*primitive_types.find(PrimitiveTypeEnum::U32), *primitive_types.find(PrimitiveTypeEnum::U64)
};

