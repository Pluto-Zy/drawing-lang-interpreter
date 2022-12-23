/**
 * This file defines the interpreter of the language.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_INTERPRETER_H
#define DRAWING_LANG_INTERPRETER_INTERPRETER_H

#include <AST/Stmt.h>
#include <AST/Expr.h>
#include <AST/StmtVisitor.h>
#include <Sema/Sema.h>
#include "InternalSupport/InternalImpl.h"

INTERPRETER_NAMESPACE_BEGIN

class interpreter : public stmt_visitor<interpreter> {
public:
  interpreter(sema& action, internal_impl& symbol)
    : action(action), symbol(symbol) { }

  /**
   * Runs all the statements in the vector and skips it
   * if we meet an error.
   */
  void run_stmts(const std::vector<stmt_result_t>& stmts);

  void visit_empty_stmt(empty_stmt*) { }
  void visit_assignment_stmt(assignment_stmt* s);
  void visit_expr_stmt(expr_stmt* s);
  void visit_for_stmt(for_stmt* s);
private:
  sema& action;
  internal_impl& symbol;
  /**
   * Helper function used to make a diagnostic message
   */
  template<class... LocTy>
  diag_builder diag(diag_id message, LocTy... locs) const {
    return action.diag(message, locs...);
  }

  bool _assign_to_value(variable_expr* lhs, typed_value& rhs,
                        std::size_t lhs_loc,
                        std::size_t rhs_start_loc, std::size_t rhs_end_loc);

  [[nodiscard]] static bool _type_assignable(const type& t);
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_INTERPRETER_H
