/**
 * This file defines the AST node class representing a statement.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_STMT_H
#define DRAWING_LANG_INTERPRETER_STMT_H

#include <Utils/StringRef.h>
#include <memory>
#include <vector>

INTERPRETER_NAMESPACE_BEGIN
class expr;

class stmt {
public:
  enum stmt_kind {
    unknown_stmt_type,
#define STMT(CLASS, PARENT) CLASS##_type,
#include "ASTNodeDef.h"
  };
protected:
  stmt_kind _kind;
  std::size_t _start_loc, _end_loc;

  stmt(stmt_kind kind,
       std::size_t start_loc,
       std::size_t end_loc)
    : _kind(kind), _start_loc(start_loc), _end_loc(end_loc) { }
public:
  stmt() = delete;
  stmt(const stmt&) = delete;
  stmt(stmt&&) = delete;
  stmt& operator=(const stmt&) = delete;
  stmt& operator=(stmt&&) = delete;
  virtual ~stmt() = default;

  [[nodiscard]] stmt_kind get_stmt_kind() const { return _kind; }
  [[nodiscard]] std::size_t get_start_loc() const { return _start_loc; }
  [[nodiscard]] std::size_t get_end_loc() const { return _end_loc; }

  [[nodiscard]] virtual bool is_expr() const { return false; }
};

using stmt_result_t = std::unique_ptr<stmt>;

/**
 * operand_stmt - represents a stmt with expressions as operands.
 */
class operand_stmt : public stmt {
protected:
  std::vector<std::unique_ptr<expr>> _operands;
  operand_stmt(stmt_kind kind, std::size_t start_loc, std::size_t end_loc,
               std::vector<std::unique_ptr<expr>> operands);
public:
  ~operand_stmt() override;
};

/**
 * assignment_stmt - represents AssignmentStatement
 *
 * There are two operands in the statement, which are the assigned
 * variable (index 0) and the value assigned to it (index 1).
 */
class assignment_stmt : public operand_stmt {
public:
  assignment_stmt(std::unique_ptr<expr> lhs, std::size_t is_loc,
                  std::unique_ptr<expr> rhs, std::size_t semi_loc);

  [[nodiscard]] expr* get_assignment_lhs() const { return _operands[0].get(); }
  [[nodiscard]] expr* get_assignment_rhs() const { return _operands[1].get(); }
  [[nodiscard]] std::size_t get_is_loc() const { return _is_loc; }
private:
  std::size_t _is_loc;
};

/**
 * for_stmt - represents ForStatement
 *
 * There are 4 operands in the statement, which are
 * the variable (index 0)
 * the 'from' expression (index 1)
 * the 'to' expression (index 2)
 * the 'step' expression (index 3)
 */
class for_stmt : public operand_stmt {
public:
  using stmt_iterator = std::vector<stmt_result_t>::iterator;
  using stmt_const_iterator = std::vector<stmt_result_t>::const_iterator;

  for_stmt(std::size_t for_loc, std::unique_ptr<expr> for_var,
           std::size_t from_loc, std::unique_ptr<expr> from_expr,
           std::size_t to_loc, std::unique_ptr<expr> to_expr,
           std::size_t step_loc, std::unique_ptr<expr> step_expr,
           std::size_t end_loc, std::vector<stmt_result_t> _body);

  [[nodiscard]] std::size_t get_for_loc() const { return _loc[FOR]; }
  [[nodiscard]] std::size_t get_from_loc() const { return _loc[FROM]; }
  [[nodiscard]] std::size_t get_to_loc() const { return _loc[TO]; }
  [[nodiscard]] std::size_t get_step_loc() const { return _loc[STEP]; }
  [[nodiscard]] std::size_t get_body_stmt_count() const { return _body.size(); }
  [[nodiscard]] bool has_from() const { return _operands[FROM] != nullptr; }
  [[nodiscard]] bool has_step() const { return _operands[STEP] != nullptr; }
  [[nodiscard]] stmt_iterator body_begin() { return _body.begin(); }
  [[nodiscard]] stmt_const_iterator body_begin() const { return _body.begin(); }
  [[nodiscard]] stmt_iterator body_end() { return _body.end(); }
  [[nodiscard]] stmt_const_iterator body_end() const { return _body.end(); }
  [[nodiscard]] expr* get_for_expr() const { return _operands[FOR].get(); }
  [[nodiscard]] expr* get_from_expr() const { return _operands[FROM].get(); }
  [[nodiscard]] expr* get_to_expr() const { return _operands[TO].get(); }
  [[nodiscard]] expr* get_step_expr() const { return _operands[STEP].get(); }
private:
  enum { FOR, FROM, TO, STEP, END };
  std::size_t _loc[END];
  std::vector<stmt_result_t> _body;
};

class empty_stmt : public stmt {
public:
  explicit empty_stmt(std::size_t semi_loc) :
    stmt(empty_stmt_type, semi_loc, semi_loc + 1) { }
};

class expr_stmt : public operand_stmt {
public:
  expr_stmt(std::unique_ptr<expr> e, std::size_t semi_loc);

  [[nodiscard]] expr* get_expr() const { return _operands[0].get(); }
};

/**
 * Returns a @code{stmt_result_t} represents an invalid statement.
 */
inline stmt_result_t stmt_error() {
  return nullptr;
}

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_STMT_H
