#include <AST/Stmt.h>
#include <AST/Expr.h>

INTERPRETER_NAMESPACE_BEGIN

template<class... Elems>
std::vector<expr_result_t> make_unique_vector(Elems&&... elems) {
  std::vector<expr_result_t> result;
  result.reserve(sizeof...(Elems));
  (result.emplace_back(std::forward<Elems>(elems)), ...);
  return result;
}

operand_stmt::operand_stmt(stmt_kind kind, std::size_t start_loc, std::size_t end_loc,
                           std::vector<std::unique_ptr<expr>> operands)
  : stmt(kind, start_loc, end_loc), _operands(std::move(operands)) { }

operand_stmt::~operand_stmt() = default;

assignment_stmt::assignment_stmt(std::unique_ptr<expr> lhs, std::size_t is_loc,
                                 std::unique_ptr<expr> rhs, std::size_t semi_loc)
  : operand_stmt(assignment_stmt_type, lhs->get_start_loc(), semi_loc + 1,
                 {} ),
  _is_loc(is_loc) { _operands.emplace_back(std::move(lhs)); _operands.emplace_back(std::move(rhs)); }

for_stmt::for_stmt(std::size_t for_loc, std::unique_ptr<expr> for_var,
                   std::size_t from_loc, std::unique_ptr<expr> from_expr,
                   std::size_t to_loc, std::unique_ptr<expr> to_expr,
                   std::size_t step_loc, std::unique_ptr<expr> step_expr,
                   std::size_t end_loc, std::vector<stmt_result_t> _body)
  : operand_stmt(for_stmt_type, for_loc, end_loc,
                 make_unique_vector(std::move(for_var), std::move(from_expr),
                                    std::move(to_expr), std::move(step_expr))),
  _loc{ for_loc, from_loc, to_loc, step_loc },
  _body(std::move(_body)) { }

expr_stmt::expr_stmt(std::unique_ptr<expr> e, std::size_t semi_loc)
  : operand_stmt(expr_stmt_type, e->get_start_loc(), semi_loc + 1,
                 {} ) { _operands.emplace_back(std::move(e)); }
INTERPRETER_NAMESPACE_END
