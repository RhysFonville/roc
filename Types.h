#pragma once

#include <iostream>
#include <memory>
#include "Lexer.h"

struct TType {
	explicit TType() : size{(uint8_t)-1} { }
	explicit TType(uint8_t size) : size{size} { }
	virtual ~TType() = default;

	virtual void print() const noexcept { }

	uint8_t get_size() { return size; }
	virtual bool is_signed() const noexcept = 0;

protected:
	uint8_t size{};
};

using Type = std::shared_ptr<TType>;

struct TConstructor : public TType {
	TConstructor() : TType{} {}
	explicit TConstructor(const RealType& type)
		: TType{type.size}, type{type} {}
	explicit TConstructor(const RealType& type, const std::vector<Type>& generics)
		: TType{type.size}, type{type}, generics{generics} {}

	bool operator==(const TConstructor& t) const noexcept {
		return (type == t.type);
	}
	bool operator!=(const TConstructor& t) const noexcept { return !(*this == t); }

	bool operator==(const RealType& t) const noexcept { return type == t; }
	bool operator!=(const RealType& t) const noexcept { return !(type == t); }

	void print() const noexcept override { std::cout << type.keyword.first << std::endl; }

	bool is_signed() const noexcept override { return type.is_signed; }

	RealType type{};
	std::vector<Type> generics{};
};

struct TVariable : public TType {
	TVariable() : TType{} {}
	explicit TVariable(int idx) : TType{}, index{idx} {}

	bool operator==(const TVariable& t) const noexcept {
		return (index == t.index);
	}
	bool operator!=(const TVariable& t) const noexcept { return !(*this == t); }

	void print() const noexcept override { std::cout << "$" << index << std::endl; }

	bool is_signed() const noexcept override { return false; }

	int index{};
};

static bool comp_types(const Type& t1, const Type& t2) noexcept;

struct TPointer : public TType {
	TPointer() : TType{sizeof(int*)} {}
	explicit TPointer(const std::shared_ptr<TType>& t) : TType{sizeof(int*)}, inner{t} {}

	bool operator==(const TPointer& t) const noexcept {
		return comp_types(inner, t.inner);
	}

	void print() const noexcept override {
		std::cout << "pointer to ";
		inner->print();
	}

	bool is_signed() const noexcept override { return false; }

	Type inner{};
};

static bool is_pointer(const Type& t) {
	return std::dynamic_pointer_cast<TPointer>(t) != nullptr;
}

static bool comp_types(const Type& t1, const Type& t2) noexcept {
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

