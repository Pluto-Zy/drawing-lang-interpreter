/**
 * This file defines the AST node class representing an expression.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_EXPR_H
#define DRAWING_LANG_INTERPRETER_EXPR_H

#include "Stmt.h"
#include "Type.h"
#include <Lex/TokenKinds.h>
#include <Sema/IdentifierInfo.h>

INTERPRETER_NAMESPACE_BEGIN

class token;

/**
 * expr - represents an expression
 * @note expression is also a statement
 */
class expr : public stmt {
protected:
  expr(stmt_kind kind, std::size_t start_loc, std::size_t end_loc )
    : stmt(kind, start_loc, end_loc) { }
public:
  [[nodiscard]] bool is_expr() const final { return true; }
  [[nodiscard]] virtual bool is_binary_expr() const { return false; }
  [[nodiscard]] virtual bool is_unary_expr() const { return false; }
};

using expr_result_t = std::unique_ptr<expr>;

class binary_expr : public expr {
public:
  enum op_kind {
    bo_unknown,
#define BIN_OP(NAME, OP, PREC, ASSOC, TOKEN) bo_##NAME,
#include "OpKindDef.h"
  };
protected:
  op_kind _op_kind;
  std::size_t _op_loc;
  expr_result_t _lhs;
  expr_result_t _rhs;

  binary_expr(op_kind kind, std::size_t loc,
              expr_result_t l, expr_result_t r)
      : expr(binary_expr_type, l->get_start_loc(), r->get_end_loc()),
        _op_kind(kind), _op_loc(loc), _lhs(std::move(l)), _rhs(std::move(r)) {}

public:
  [[nodiscard]] op_kind get_op_kind() const { return _op_kind; }
  [[nodiscard]] bool is_binary_expr() const final { return true; }
  [[nodiscard]] std::size_t get_operator_loc() const { return _op_loc; }
  [[nodiscard]] expr* get_lhs() const { return _lhs.get(); }
  [[nodiscard]] expr* get_rhs() const { return _rhs.get(); }
  static string_ref get_op_str(op_kind kind);
  [[nodiscard]] string_ref get_op_str() const { return get_op_str(_op_kind); }
  [[nodiscard]] std::size_t get_op_loc() const { return _op_loc; }
  /**
   * Converts @code{token_kind} to @code{binary_expr::op_kind}.
   */
  static op_kind token_kind_to_op_kind(token_kind kind);

  /**
   * Creates a @code{binary_expr} and returns a @code{unique_ptr}
   * pointing to it.
   *
   * It will accept a @code{token} and convert the @code{token_kind} to
   * @code{binary_expr::op_kind}.
   *
   * @note @param{lhs} and @param{rhs} must not be empty and the function
   * will not check them.
   */
  static std::unique_ptr<binary_expr>
  create_binary_op(const token& tok, expr_result_t lhs, expr_result_t rhs);
};

class unary_expr : public expr {
public:
  enum op_kind {
    uo_unknown,
#define UNARY_OP(NAME, OP, PREC, ASSOC, TOKEN) uo_##NAME,
#include "OpKindDef.h"
  };
protected:
  op_kind _op_kind;
  std::size_t _op_loc;
  expr_result_t _operand;

  unary_expr(op_kind kind, std::size_t loc, expr_result_t operand)
    : expr(unary_expr_type,
           // FIXME: If the operator has more than one characters,
           //  we should mark the end of the operator correctly.
           is_prefix(kind) ? loc : operand->get_start_loc(),
           is_prefix(kind) ? operand->get_end_loc() : loc + 1),
    _op_kind(kind), _op_loc(loc), _operand(std::move(operand)) { }
public:
  [[nodiscard]] op_kind get_op_kind() const { return _op_kind; }
  [[nodiscard]] bool is_unary_expr() const final { return true; }
  [[nodiscard]] std::size_t get_operator_loc() const { return _op_loc; }
  [[nodiscard]] expr* get_operand() const { return _operand.get(); }

  // All unary operators in the language are prefix now.
  static bool is_postfix(op_kind kind) { (void)kind; return false; }
  static bool is_prefix(op_kind kind) { (void)kind; return true; }

  static string_ref get_op_str(op_kind kind);
  [[nodiscard]] string_ref get_op_str() const { return get_op_str(_op_kind); }
  /**
   * Converts @code{token_kind} to @code{unary_expr::op_kind}.
   */
  static op_kind token_kind_to_op_kind(token_kind kind);

  /**
   * Creates a @code{unary_expr} and returns a @code{unique_ptr}
   * pointing to it.
   *
   * It will accept a @code{token} and convert the @code{token_kind} to
   * @code{unary_expr::op_kind}.
   *
   * @note @param{operand} must not be empty and the function
   * will not check it.
   */
  static std::unique_ptr<unary_expr>
  create_unary_op(const token& tok, expr_result_t operand);
};

/**
 * variable_expr - represents a variable or a constant.
 *
 * The AST node will save the name of the variable (or constant). In
 * the parsing phase, the value of @code{variable_info} will not be
 * set: it is set in the semantic analysis phase.
 */
class variable_expr : public expr {
  string_ref _var_name;
  variable_info* _info;
public:
  variable_expr(string_ref name, std::size_t start_loc, std::size_t end_loc) :
    expr(stmt_kind::variable_expr_type, start_loc, end_loc),
    _var_name(name), _info(nullptr) { }

  [[nodiscard]] string_ref get_name() const
    { return _var_name; }

  [[nodiscard]] bool has_bind_info() const { return _info != nullptr; }
  void bind_to_variable(variable_info* info) {
    assert(!has_bind_info());
    _info = info;
  }
  [[nodiscard]] std::any get_bind_value() const { return _info->get_value(); }
  [[nodiscard]] const type& get_bind_type() const { return _info->get_type(); }
  [[nodiscard]] variable_info& get_bind_info() const { return *_info; }
};

class num_expr : public expr {
  double value;
  /**
   * Indicates whether the value in the source code contains a decimal
   * point. If it contains a decimal point, the value will be deduced
   * as a floating point number.
   */
  bool _has_float_point;
public:
  num_expr(double value, std::size_t start_loc, std::size_t end_loc, bool float_point) :
    expr(num_expr_type, start_loc, end_loc), value(value), _has_float_point(float_point) { }

  [[nodiscard]] double get_value() const { return value; }
  [[nodiscard]] bool has_float_point() const { return _has_float_point; }
};

class string_expr : public expr {
  std::string value;
public:
  string_expr(std::string value, std::size_t start_loc, std::size_t end_loc)
    : expr(string_expr_type, start_loc, end_loc), value(std::move(value)) { }

  [[nodiscard]] const std::string& get_value() const { return value; }
};

class tuple_expr : public expr {
  std::size_t l_paren_loc;
  std::size_t r_paren_loc;
  std::vector<std::unique_ptr<expr>> _elems;
public:
  using elem_list_t = std::vector<std::unique_ptr<expr>>;
  using elem_const_iterator = elem_list_t::const_iterator;
  using elem_iterator = elem_list_t::iterator;

  tuple_expr(elem_list_t elems, std::size_t l_paren_loc, std::size_t r_paren_loc)
    : expr(tuple_expr_type, l_paren_loc, r_paren_loc + 1),
    _elems(std::move(elems)), l_paren_loc(l_paren_loc), r_paren_loc(r_paren_loc) { }

  [[nodiscard]] std::size_t get_elem_count() const { return _elems.size(); }
  [[nodiscard]] expr* get_elem(std::size_t idx) const { return _elems[idx].get(); }
  [[nodiscard]] elem_const_iterator elem_begin() const { return _elems.cbegin(); }
  [[nodiscard]] elem_iterator elem_begin() { return _elems.begin(); }
  [[nodiscard]] elem_const_iterator elem_end() const { return _elems.cend(); }
  [[nodiscard]] elem_iterator elem_end() { return _elems.end(); }
  [[nodiscard]] std::size_t get_l_paren_loc() const { return l_paren_loc; }
  [[nodiscard]] std::size_t get_r_paren_loc() const { return r_paren_loc; }
};

/**
 * call_expr - represents a function call expression.
 *
 * In the parsing phase, the value of @code{function_info} will not be
 * set: it is set in the semantic analysis phase.
 */
class call_expr : public expr {
public:
  using param_list_t = std::vector<std::unique_ptr<expr>>;
  using param_const_iterator = param_list_t::const_iterator;
  using param_iterator = param_list_t::iterator;

  call_expr(string_ref func_name, param_list_t params,
            std::size_t func_loc, std::size_t l_paren_loc, std::size_t r_paren_loc) :
    expr(stmt_kind::call_expr_type, func_loc, r_paren_loc + 1),
    _func_name(func_name), _args(std::move(params)),
    _locs{ func_loc, l_paren_loc, r_paren_loc }, _info(nullptr) { }

  [[nodiscard]] string_ref get_func_name() const { return _func_name; }
  [[nodiscard]] std::size_t get_param_count() const { return _args.size(); }
  [[nodiscard]] expr* get_arg_expr(std::size_t idx) const { return _args[idx].get(); }
  [[nodiscard]] param_const_iterator param_begin() const { return _args.begin(); }
  [[nodiscard]] param_iterator param_begin() { return _args.begin(); }
  [[nodiscard]] param_const_iterator param_end() const { return _args.end(); }
  [[nodiscard]] param_iterator param_end() { return _args.end(); }

  [[nodiscard]] std::size_t get_func_name_loc() const { return _locs[FUNC_NAME]; }
  [[nodiscard]] std::size_t get_l_paren_loc() const { return _locs[L_PAREN]; }
  [[nodiscard]] std::size_t get_r_paren_loc() const { return _locs[R_PAREN]; }

  [[nodiscard]] bool has_bind_info() const { return _info != nullptr; }
  void bind_to_function(const function_info* info) {
    assert(!has_bind_info());
    _info = info;
  }
  [[nodiscard]] const function_info& get_bind_func() const { return *_info; }
private:
  string_ref _func_name;
  param_list_t _args;
  enum { FUNC_NAME, L_PAREN, R_PAREN, END };
  std::size_t _locs[END];
  const function_info* _info;
};

/**
 * Returns a @code{expr_result_t} represents an invalid expression.
 */
inline expr_result_t expr_error() {
  return nullptr;
}

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_EXPR_H
