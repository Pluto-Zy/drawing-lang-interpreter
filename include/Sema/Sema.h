#ifndef DRAWING_LANG_INTERPRETER_SEMA_H
#define DRAWING_LANG_INTERPRETER_SEMA_H

#include <AST/Stmt.h>
#include <AST/Expr.h>
#include <AST/Type.h>
#include <Diagnostic/DiagEngine.h>
#include "IdentifierInfo.h"
#include <Interpret/TypedValue.h>

INTERPRETER_NAMESPACE_BEGIN

/**
 * Used for semantic analysis and AST building.
 */
class sema {
  using integer_type = int;
  /**
   * Used to report diag message.
   */
  diag_engine& _diag_engine;
  /**
   * Predefined variables and functions.
   */
  symbol_table& _symbol_table;
public:
  sema(diag_engine& diag, symbol_table& table);
  sema(const sema&) = delete;
  sema(sema&&) = delete;
  sema& operator=(const sema&) = delete;
  sema& operator=(sema&&) = delete;
  /**
   * Helper function used to make a diagnostic message
   */
  template<class... LocTy>
  diag_builder diag(diag_id message, LocTy... locs) const {
    return _diag_engine.create_diag(message, locs...);
  }

  [[nodiscard]] diag_engine& get_diag_engine() const { return _diag_engine; }
  [[nodiscard]] symbol_table& get_symbol_table() const { return _symbol_table; }

  /**
   * Use name lookup to bind all variable (without function) names in
   * the expression to the corresponding entities. If the variable
   * does not exist, report an error and return false, otherwise return true.
   */
  bool bind_expr_variables(expr* e);

  /**
   * Binds variable names to the corresponding entities and don't throw diagnostics
   * on failure.
   */
  bool try_bind_expr_variables(expr* e);

  [[nodiscard]] variable_info* add_new_variable(typed_value init_value, string_ref variable_name);

  /**
   * Evaluates the AST of the expression, checks the type of the operands,
   * and performs appropriate type conversions if necessary. For function
   * call expressions, overload resolution is required. Before calling
   * this function, @code{bind_expr_variables} must be called to bind the
   * name to the entity.
   *
   * If @param{simplify} is @code{true}, constant folding will be performed
   * during the evaluation process to simplify the structure of the AST
   * (this is often used to simplify expressions in the for loop body to
   * improve efficiency).
   *
   * If the type of the operands of the expression is correct and the
   * function overload resolution is clear, the corresponding
   * type-expression pair is returned.
   */
  std::optional<typed_value> evaluate(expr* e, bool simplify = false);

  /**
   * Checks whether the @param{value} can be represented
   * by an @code{int}.
   */
  static bool check_double_to_int(double value);

  [[nodiscard]] std::optional<type>
  find_common_type(const type& lhs, const type& rhs) const;

  [[nodiscard]] std::optional<typed_value>
  tidy_tuple(const std::vector<typed_value>& tuple_elems,
             std::vector<std::size_t> elem_loc) const;

  [[nodiscard]] typed_value
  convert_to(typed_value from, const type& to, bool& narrow) const;
  [[nodiscard]] std::vector<std::any>
  convert_to(std::vector<std::any> values, const type& src, const type& dst, bool& narrow) const;
  [[nodiscard]] bool can_convert_to(const type& from, const type& to) const;

private:
  [[nodiscard]] int get_match_level(const type& arg, const type& param) const;

  [[nodiscard]] std::vector<const function_info*>
  _get_candidate_functions(string_ref func_name, std::size_t func_name_loc) const;

  [[nodiscard]] std::vector<const function_info*>
  _get_viable_functions(std::vector<const function_info*> candidates,
                        const std::vector<const type*>& param_types,
                        string_ref func_name,
                        std::size_t func_name_loc) const;

  [[nodiscard]] const function_info*
  _find_best_viable_function(std::vector<const function_info*> viable_funcs,
                             const std::vector<const type*>& param_types,
                             string_ref func_name,
                             std::size_t func_name_loc) const;
public:
  [[nodiscard]] const function_info*
  overload_resolution(string_ref func_name, std::size_t func_name_loc,
                      const std::vector<const type*>& param_types) const;
  [[nodiscard]] const function_info*
  overload_resolution(string_ref func_name, std::size_t func_name_loc,
                      const std::vector<typed_value>& param) const;

private:
  [[nodiscard]] std::pair<FLOAT_POINT_T, FLOAT_POINT_T>
  _extract_basic_integer_value(const type& lhs_type, std::any lhs_value,
                               const type& rhs_type, std::any rhs_value) const;
  [[nodiscard]] FLOAT_POINT_T
  _extract_basic_integer_value(const type& operand_type, std::any operand_value) const;

  [[nodiscard]] STRING_T
  _extract_basic_string_value(const type& operand_type, std::any operand_value) const;

  [[nodiscard]] std::pair<STRING_T, STRING_T>
  _extract_basic_string_value(const type& lhs_type, std::any lhs_value,
                              const type& rhs_type, std::any rhs_value) const;

  [[nodiscard]] std::optional<typed_value>
  _binary_on_basic_num_type(const type& lhs_type, std::any lhs,
                            const type& rhs_type, std::any rhs,
                            std::size_t op_loc, binary_expr::op_kind kind) const;
  [[nodiscard]] std::optional<typed_value>
  _binary_on_basic_type(const type& lhs_type, std::any lhs,
                        const type& rhs_type, std::any rhs,
                        std::size_t op_loc, binary_expr::op_kind kind) const;
  [[nodiscard]] std::optional<typed_value>
  _unary_on_basic_type(const type& op_type, std::any operand,
                       std::size_t op_loc, unary_expr::op_kind kind) const;

  using _binary_op_impl_t =
      std::optional<typed_value>(const type&, std::any, const type&, std::any, std::size_t);
  [[nodiscard]] std::optional<typed_value>
  _binary_on_tuple_num(const type& lhs_type, std::any lhs_value,
                       const type& rhs_type, std::any rhs_value,
                       std::size_t op_loc, std::function<_binary_op_impl_t> impl) const;
  using _unary_op_impl_t =
      std::optional<typed_value>(const type&, std::any, std::size_t);
  [[nodiscard]] std::optional<typed_value>
  _unary_on_tuple_elem(const type& operand_type, std::any operand_value,
                       std::size_t op_loc, std::function<_unary_op_impl_t> impl) const;

public:
#define BINARY_OP_IMPL(OP_NAME)                                           \
[[nodiscard]] bool can_##OP_NAME(const type& lhs, const type& rhs) const; \
                                                                          \
[[nodiscard]] std::optional<typed_value>                                  \
_##OP_NAME##_unchecked(const type& lhs_type, std::any lhs_value,          \
                       const type& rhs_type, std::any rhs_value,          \
                       std::size_t op_loc) const;

  BINARY_OP_IMPL(add)
  BINARY_OP_IMPL(sub)
  BINARY_OP_IMPL(mul)
  BINARY_OP_IMPL(div)
  BINARY_OP_IMPL(pow)

#undef BINARY_OP_IMPL

#define UNARY_OP_IMPL(OP_NAME)                                            \
[[nodiscard]] bool can_##OP_NAME(const type& op) const;                   \
                                                                          \
[[nodiscard]] std::optional<typed_value>                                  \
_##OP_NAME##_unchecked(const type& op_type, std::any op_value,            \
                       std::size_t op_loc) const;

  UNARY_OP_IMPL(unary_plus)
  UNARY_OP_IMPL(unary_minus)

#undef UNARY_OP_IMPL
private:
  int _compare_basic_type(const type& lhs_type, std::any lhs_value,
                          const type& rhs_type, std::any rhs_value) const;
public:
  int compare(const type& lhs_type, std::any lhs_value,
              const type& rhs_type, std::any rhs_value,
              std::size_t op_loc);
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_SEMA_H
