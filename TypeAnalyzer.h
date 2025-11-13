#include "Syntax.h"
#include "Types.h"
#include "Environment.h"
#include <memory>

struct Constraint {
	virtual ~Constraint() = default;
};

struct CEquality : public Constraint {
	CEquality() : Constraint{} { }
	explicit CEquality(const TType_ptr& t1, const TType_ptr& t2) : Constraint{}, t1{t1}, t2{t2} { }
	TType_ptr t1{};
	TType_ptr t2{};
};

static bool is_inferred(const TType_ptr& t) {
	if (std::dynamic_pointer_cast<TConstructor>(t) != nullptr) return true;
	if (auto p{std::dynamic_pointer_cast<TPointer>(t)}) return is_inferred(p->inner);
	return false;
}


class TypeAnalyzer {
public:
	TypeAnalyzer(const std::vector<std::shared_ptr<Statement>>& stmts);

	bool run();

private:
	std::vector<std::shared_ptr<Statement>> stmts{};

	std::vector<TType_ptr> substitution{};
	std::vector<std::shared_ptr<Constraint>> type_constraints{};

	bool success{true};

	void type_error(const Token& token, const std::string& message);

	TType_ptr fresh_type_variable() {
		TType_ptr result{std::make_shared<TVariable>((int)substitution.size())};
		substitution.push_back(result);
		return substitution.back();
	}
	bool occurs_in(int index, const TType_ptr& type);
	void unify(const TType_ptr& t1, const TType_ptr& t2);

	TType_ptr substitute(const TType_ptr& type);

	void solve_constraints();

	void resolve_parse_type(MixType& type);
	void translate_ttype_ptr(MixType& type);

	void infer_expression(const std::shared_ptr<Expression>& expr);
	void infer_identifier_expression(const std::shared_ptr<IdentifierExpression>& expr);
	void infer_literal_expression(const std::shared_ptr<LiteralExpression>& expr);
	void infer_grouping_expression(const std::shared_ptr<GroupingExpression>& expr);
	void infer_unary_expression(const std::shared_ptr<UnaryExpression>& expr);
	void infer_binary_expression(const std::shared_ptr<BinaryExpression>& expr);
	void infer_block_expression(const std::shared_ptr<BlockExpression>& expr, const std::shared_ptr<FunctionDeclarationStatement>& func = nullptr);
	void infer_call_expression(const std::shared_ptr<CallExpression>& expr);
	void infer_return_expression(const std::shared_ptr<ReturnExpression>& expr);
	void infer_cast_expression(const std::shared_ptr<CastExpression>& expr);
	void infer_type_expression(const std::shared_ptr<TypeExpression>& expr);
	
	void infer_statement(const std::shared_ptr<Statement>& stmt);
	void infer_expression_statement(const std::shared_ptr<ExpressionStatement>& stmt);
	void infer_variable_declaration_statement(const std::shared_ptr<VariableDeclarationStatement>& stmt);
	void infer_function_declaration_statement(const std::shared_ptr<FunctionDeclarationStatement>& stmt);

	void substitute_expression(const std::shared_ptr<Expression>& expr);
	void substitute_identifier_expression(const std::shared_ptr<IdentifierExpression>& expr);
	void substitute_literal_expression(const std::shared_ptr<LiteralExpression>& expr);
	void substitute_grouping_expression(const std::shared_ptr<GroupingExpression>& expr);
	void substitute_unary_expression(const std::shared_ptr<UnaryExpression>& expr);
	void substitute_binary_expression(const std::shared_ptr<BinaryExpression>& expr);
	void substitute_block_expression(const std::shared_ptr<BlockExpression>& expr);
	void substitute_call_expression(const std::shared_ptr<CallExpression>& expr);
	void substitute_return_expression(const std::shared_ptr<ReturnExpression>& expr);
	void substitute_cast_expression(const std::shared_ptr<CastExpression>& expr);
	void substitute_type_expression(const std::shared_ptr<TypeExpression>& expr);
	
	void substitute_statement(const std::shared_ptr<Statement>& stmt);
	void substitute_expression_statement(const std::shared_ptr<ExpressionStatement>& stmt);
	void substitute_variable_declaration_statement(const std::shared_ptr<VariableDeclarationStatement>& stmt);
	void substitute_function_declaration_statement(const std::shared_ptr<FunctionDeclarationStatement>& stmt);

	EnvironmentStack env_stack{};
};

