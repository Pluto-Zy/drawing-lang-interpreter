#include <Sema/Sema.h>
#include <AST/StmtVisitor.h>
#include <iterator>
#include <Diagnostic/DiagData.h>
#include <cmath>
#include <sstream>

INTERPRETER_NAMESPACE_BEGIN

namespace {
class var_binding_visitor : public stmt_visitor<var_binding_visitor, bool> {
public:
  var_binding_visitor(symbol_table& table, diag_engine& diag, bool make_diag)
      : _table(table), _diag(diag), _make_diag(make_diag) { }

  bool visit_binary_expr(binary_expr* e) {
    assert(e);
    // Prevent short circuit evaluation.
    bool l_value = visit(e->get_lhs());
    bool r_value = visit(e->get_rhs());
    return l_value && r_value;
  }

  bool visit_unary_expr(unary_expr* e) {
    assert(e);
    return visit(e->get_operand());
  }

  bool visit_variable_expr(variable_expr* e) {
    assert(e);
    if (e->has_bind_info())
      return true;
    // find the name of the variable in the symbol table
    variable_info* var_info = _table.get_variable(e->get_name());
    if (var_info) {
      e->bind_to_variable(var_info);
      return true;
    }
    // If we don't need to make diagnostics, no more checks are needed.
    if (!_make_diag)
      return false;
    // check whether it is a function
    if (_table.has_function(e->get_name())) {
      _diag.create_diag(err_use_func_as_var, e->get_start_loc()) << diag_build_finish;
      return false;
    }
    // check whether it is a typo
    if (auto result = _check_variable_typo(e->get_name())) {
      _diag.create_diag(err_use_unknown_identifier_with_hint, e->get_start_loc())
          << result->first << _diag.create_replacement(e->get_start_loc(), e->get_end_loc(), result->first)
          << diag_build_finish;
      e->bind_to_variable(result->second);
      return true;
    }
    _diag.create_diag(err_use_unknown_identifier, e->get_start_loc()) << diag_build_finish;
    return false;
  }

  bool visit_num_expr(num_expr* e) {
    assert(e);
    (void)e;
    return true;
  }

  bool visit_string_expr(string_expr* e) {
    assert(e);
    (void)e;
    return true;
  }

  bool visit_tuple_expr(tuple_expr* e) {
    assert(e);
    bool result = true;
    for (auto iter = e->elem_begin(); iter != e->elem_end(); ++iter) {
      result &= visit(iter->get());
    }
    return result;
  }

  bool visit_call_expr(call_expr* e) {
    assert(e);
    bool result = true;
    for (auto iter = e->param_begin(); iter != e->param_end(); ++iter) {
      result &= visit(iter->get());
    }
    return result;
  }

private:
  symbol_table& _table;
  diag_engine& _diag;
  bool _make_diag;

  std::optional<std::pair<string_ref, variable_info*>>
  _check_variable_typo(string_ref spelling) {
    auto _distance_checker =
        [_min_distance = 5u, spelling] (string_ref name, variable_info* info) mutable {
            unsigned distance = spelling.edit_distance(name);
            if (distance <= _min_distance && distance < std::min(spelling.size(), name.size())) {
              _min_distance = distance;
              return true;
            }
            return false;
        };
    std::vector<std::pair<string_ref, variable_info*>> _candidates;
    _table.get_var_if(std::back_inserter(_candidates), _distance_checker);
    if (_candidates.size() == 1) {
      return _candidates.back();
    } else if (_candidates.size() > 1) {
      if (spelling.edit_distance(_candidates.back().first) <
          spelling.edit_distance(_candidates[_candidates.size() - 2].first)) {
        return _candidates.back();
      }
    }
    return std::nullopt;
  }
};
}

bool sema::bind_expr_variables(expr* e) {
  var_binding_visitor visitor(_symbol_table, _diag_engine, /* make_diag = */true);
  return visitor.visit(e);
}

bool sema::try_bind_expr_variables(expr* e) {
  var_binding_visitor visitor(_symbol_table, _diag_engine, /* make_diag = */false);
  return visitor.visit(e);
}

namespace {
using RetTy = std::optional<typed_value>;

class expr_eval_visitor : public stmt_visitor<expr_eval_visitor, RetTy> {
public:
  expr_eval_visitor(sema& s, bool simplify = false)
    : _simplify(simplify), action(s) { }

#define BINARY_OP_FUNC(OP_NAME)                                                                     \
RetTy visit_binary_##OP_NAME##_op(binary_expr* e) {                                                 \
  assert(e);                                                                                        \
  auto [lhs, rhs] = _evaluate_binary_operands(e);                                                   \
  if (!lhs || !rhs)                                                                                 \
    return std::nullopt;                                                                            \
  if (!action.can_##OP_NAME(lhs->get_type(), rhs->get_type())) {                                    \
    action.diag(err_invalid_binary_operand, e->get_op_loc())                                        \
        << lhs->get_type().get_spelling() << rhs->get_type().get_spelling() << diag_build_finish;   \
    return std::nullopt;                                                                            \
  }                                                                                                 \
  auto _op_result =                                                                                 \
      action._##OP_NAME##_unchecked(lhs->get_type(), lhs->take_value(),                             \
                                    rhs->get_type(), rhs->take_value(),                             \
                                    e->get_op_loc());                                               \
  if (!_op_result)                                                                                  \
    return std::nullopt;                                                                            \
  if (lhs->is_constant() && rhs->is_constant())                                                     \
    _op_result->make_constant();                                                                    \
  return _op_result;                                                                                \
}

#define UNARY_OP_FUNC(OP_NAME)                                          \
RetTy visit_unary_##OP_NAME##_op(unary_expr* e) {                       \
  assert(e);                                                            \
  auto operand = visit(e->get_operand());                               \
  if (!operand)                                                         \
    return std::nullopt;                                                \
  if (!action.can_unary_##OP_NAME(operand->get_type())) {               \
    action.diag(err_invalid_unary_operand, e->get_operator_loc())       \
        << operand->get_type().get_spelling() << diag_build_finish;     \
    return std::nullopt;                                                \
  }                                                                     \
  auto _op_result =                                                     \
      action._unary_##OP_NAME##_unchecked(operand->get_type(),          \
                                          operand->take_value(),        \
                                          e->get_operator_loc());       \
  if (!_op_result)                                                      \
    return std::nullopt;                                                \
  if (operand->is_constant())                                           \
    _op_result->make_constant();                                        \
  return _op_result;                                                    \
}

#define BIN_OP(NAME, OP, PREC, ASSOC, KIND) BINARY_OP_FUNC(NAME)
#define UNARY_OP(NAME, OP, PREC, ASSOC, KIND) UNARY_OP_FUNC(NAME)
#include <AST/OpKindDef.h>

  RetTy visit_variable_expr(variable_expr* e) {
    assert(e && e->has_bind_info());
    const variable_info& info = e->get_bind_info();
    if (info.is_constant())
      return make_constant_typed_value(info.get_type(), info.get_value());
    return make_typed_value(info.get_type(), info.get_value());
  }

  RetTy visit_num_expr(num_expr* e) {
    assert(e);
    auto value = e->get_value();
    if (!e->has_float_point() && action.check_double_to_int(value)) {
      return make_constant_typed_value(make_INTEGER_type(),
                                       static_cast<INTEGER_T>(value));
    }
    return make_constant_typed_value(make_FLOAT_POINT_type(), value);
  }

  RetTy visit_string_expr(string_expr* e) {
    assert(e);
    auto value = e->get_value();
    return make_constant_typed_value(make_STRING_type(), std::move(value));
  }

  RetTy visit_tuple_expr(tuple_expr* e) {
    assert(e);
    assert(e->get_elem_count() > 1);
    std::vector<typed_value> result_value = _evaluate_exprs(e->elem_begin(), e->elem_end());
    if (result_value.size() != e->get_elem_count())
      return std::nullopt;
    std::vector<std::size_t> loc;
    loc.reserve(e->get_elem_count());
    for (auto iter = e->elem_begin(); iter != e->elem_end(); ++iter) {
      loc.push_back(e->get_start_loc());
    }
    return action.tidy_tuple(result_value, std::move(loc));
  }

  RetTy visit_call_expr(call_expr* e) {
    assert(e);
    // evaluate params
    std::vector<typed_value> params = _evaluate_exprs(e->param_begin(), e->param_end());
    if (params.size() != e->get_param_count())
      return std::nullopt;
    // If there is no binding function information,
    // we need to make an overload resolution first.
    if (!e->has_bind_info()) {
      const function_info* result =
          action.overload_resolution(e->get_func_name(), e->get_func_name_loc(), params);
      if (!result)
        return std::nullopt;
      e->bind_to_function(result);
    }
    const function_info& bind_info = e->get_bind_func();
    // convert arguments to the type of corresponding parameter
    std::vector<std::any> arguments;
    arguments.reserve(e->get_param_count());
    for (std::size_t i = 0; i < params.size(); ++i) {
      std::string _origin_value = params[i].get_value_spelling();
      std::string _origin_type = params[i].get_type().get_spelling();
      bool narrow = false;
      typed_value _converted_result =
          action.convert_to(std::move(params[i]), bind_info.get_param_type(i), narrow);
      if (narrow) {
        action.diag(warn_narrow_conversion,
                    e->get_arg_expr(i)->get_start_loc(),
                    e->get_arg_expr(i)->get_end_loc())
          << _origin_type << _converted_result.get_type().get_spelling()
          << _origin_value << _converted_result.get_value_spelling() << diag_build_finish;
      }
      arguments.emplace_back(std::move(_converted_result.take_value()));
    }
    // prepare param loc
    std::vector<std::size_t> param_loc;
    param_loc.reserve(2 * e->get_param_count());
    for (auto iter = e->param_begin(); iter != e->param_end(); ++iter) {
      param_loc.push_back((*iter)->get_start_loc());
      param_loc.push_back((*iter)->get_end_loc());
    }
    diag_info_pack pack { action.get_diag_engine(), std::move(param_loc), true };
    std::any call_result = e->get_bind_func().call(pack, std::move(arguments));
    if (pack.success)
      return make_typed_value(bind_info.get_ret_type(), std::move(call_result));
    return std::nullopt;
  }
private:
  bool _simplify;
  sema& action;

  template<class Ty, std::enable_if_t<!std::is_same_v<Ty, std::any>, int> = 0>
  static typed_value make_constant_typed_value(type t, Ty v) {
    return typed_value(std::move(t), std::make_any<Ty>(std::move(v)), /* constant = */true);
  }

  static typed_value make_constant_typed_value(type t, std::any v) {
    return typed_value(std::move(t), std::move(v), /* constant = */true);
  }

  static typed_value make_typed_value(type t, std::any v) {
    return typed_value(std::move(t), std::move(v), /* constant = */false);
  }

  template<class Iter>
  std::vector<typed_value> _evaluate_exprs(Iter beg, Iter end) {
    static_assert(std::is_same_v<typename std::iterator_traits<Iter>::value_type,
        std::unique_ptr<expr>>, "invalid iterator value_type used for evaluate");
    std::vector<typed_value> result;
    if constexpr (std::is_same_v<typename std::iterator_traits<Iter>::iterator_category,
        std::random_access_iterator_tag>) {
      result.reserve(end - beg);
    }
    for (; beg != end; ++beg) {
      assert(*beg);
      RetTy elem = visit(beg->get());
      if (!elem)
        continue;
      result.emplace_back(std::move(*elem));
    }
    return result;
  }

  std::pair<RetTy, RetTy> _evaluate_binary_operands(binary_expr* e) {
    return { visit(e->get_lhs()), visit(e->get_rhs()) };
  }

#define BASIC_TYPE(NAME, TYPE, SPELLING) \
static type make_##NAME##_type()         \
{ return type(type::NAME); }
#include <Interpret/TypeDef.h>
};
}

std::optional<typed_value> sema::evaluate(expr* e, bool simplify) {
  expr_eval_visitor visitor(*this);
  return visitor.visit(e);
}

std::pair<FLOAT_POINT_T, FLOAT_POINT_T>
sema::_extract_basic_integer_value(const type& lhs_type, std::any lhs_value,
                                   const type& rhs_type, std::any rhs_value) const {
  return {_extract_basic_integer_value(lhs_type, std::move(lhs_value)),
          _extract_basic_integer_value(rhs_type, std::move(rhs_value))};
}

FLOAT_POINT_T
sema::_extract_basic_integer_value(const type& operand_type,
                                   std::any operand_value) const {
  if (operand_type.is(type::FLOAT_POINT))
    return unpack_value<FLOAT_POINT_T>(std::move(operand_value));
  else {
    assert(operand_type.is(type::INTEGER));
    return static_cast<FLOAT_POINT_T>(unpack_value<INTEGER_T>(std::move(operand_value)));;
  }
}

STRING_T
sema::_extract_basic_string_value(const type& operand_type,
                                  std::any operand_value) const {
  if (operand_type.is(type::STRING))
    return unpack_value<STRING_T>(std::move(operand_value));
  if (operand_type.is(type::INTEGER))
    return std::to_string(unpack_value<INTEGER_T>(std::move(operand_value)));
  std::stringstream s;
  s.precision(15);
  s << unpack_value<FLOAT_POINT_T>(std::move(operand_value));
  return s.str();
}

std::pair<STRING_T, STRING_T>
sema::_extract_basic_string_value(const type& lhs_type, std::any lhs_value,
                                  const type& rhs_type, std::any rhs_value) const {
  return {_extract_basic_string_value(lhs_type, std::move(lhs_value)),
          _extract_basic_string_value(rhs_type, std::move(rhs_value))};
}

std::optional<typed_value>
sema::_binary_on_basic_num_type(const type& lhs_type, std::any lhs,
                                const type& rhs_type, std::any rhs,
                                std::size_t op_loc, binary_expr::op_kind kind) const {
  static_assert(std::numeric_limits<FLOAT_POINT_T>::is_iec559,
                "Only support IEEE-754 currently.");
  auto [lhs_value, rhs_value] = _extract_basic_integer_value(lhs_type, std::move(lhs),
                                                             rhs_type, std::move(rhs));

  FLOAT_POINT_T result = 0;
  std::string op_name;
  // For n-th power operations, we cannot conclude that the result
  // must be a floating-point number based on the fact that the
  // right operand is a floating-point number. For example,
  // the result of 4 ** 0.5 is 2, which is an integer.
  bool should_check = kind == binary_expr::bo_pow ?
                      lhs_type.is(type::INTEGER) :
                      lhs_type.is(type::INTEGER) && rhs_type.is(type::INTEGER);
  switch (kind) {
    case binary_expr::bo_add:
      op_name = "adding";
      result = lhs_value + rhs_value;
      break;
    case binary_expr::bo_sub:
      op_name = "subtracting";
      result = lhs_value - rhs_value;
      break;
    case binary_expr::bo_mul:
      op_name = "multiplying";
      result = lhs_value * rhs_value;
      break;
    case binary_expr::bo_div:
      if (rhs_value == 0) {
        diag(warn_div_zero, op_loc) << diag_build_finish;
      }
      op_name = "dividing";
      result = lhs_value / rhs_value;
      break;
    case binary_expr::bo_pow:
      op_name = "powering";
      result = std::pow(lhs_value, rhs_value);
      break;
    default:
      assert(false);
  }
  if (std::isinf(result) || std::isnan(result)) {
    diag(err_invalid_binary_result, op_loc)
        << op_name << lhs_value << rhs_value << diag_build_finish;
    return std::nullopt;
  }

  if (should_check && check_double_to_int(result)) {
    return typed_value(type(type::INTEGER), static_cast<INTEGER_T>(result), false);
  }
  return typed_value(type(type::FLOAT_POINT), result, false);
}

std::optional<typed_value>
sema::_binary_on_basic_type(const type& lhs_type, std::any lhs,
                            const type& rhs_type, std::any rhs,
                            std::size_t op_loc, binary_expr::op_kind kind) const {
  if (lhs_type.is_not(type::STRING) && rhs_type.is_not(type::STRING))
    return _binary_on_basic_num_type(lhs_type, std::move(lhs),
                                     rhs_type, std::move(rhs),
                                     op_loc, kind);
  if (lhs_type.is(type::STRING) && rhs_type.is(type::STRING)) {
    auto [l, r] = _extract_basic_string_value(lhs_type, std::move(lhs),
                                              rhs_type, std::move(rhs));
    return typed_value(type(type::STRING), l + r, false);
  }
  STRING_T str_value = lhs_type.is(type::STRING) ? _extract_basic_string_value(lhs_type, std::move(lhs))
                                                 : _extract_basic_string_value(rhs_type, std::move(rhs));
  if (kind == binary_expr::bo_add) {
    STRING_T num_value = lhs_type.is(type::STRING) ? _extract_basic_string_value(rhs_type, std::move(rhs))
                                                   : _extract_basic_string_value(lhs_type, std::move(lhs));
    return typed_value(type(type::STRING),
                       lhs_type.is(type::STRING) ? str_value + num_value : num_value + str_value, false);
  } else {
    assert(kind == binary_expr::bo_mul);
    INTEGER_T num_value = unpack_value<INTEGER_T>(std::move(lhs_type.is(type::STRING) ? rhs : lhs));
    if (num_value < 0) {
      diag(err_mul_str_negative_num, op_loc) << std::to_string(num_value) << diag_build_finish;
      return std::nullopt;
    }
    STRING_T result;
    for (INTEGER_T i = 0; i < num_value; ++i) {
      result += str_value;
    }
    return typed_value(type(type::STRING), std::move(result), false);
  }
}

std::optional<typed_value>
sema::_unary_on_basic_type(const type& op_type, std::any operand,
                           std::size_t op_loc, unary_expr::op_kind kind) const {
  assert(op_type.is_not(type::STRING));
  FLOAT_POINT_T operand_value = _extract_basic_integer_value(op_type, std::move(operand));
  switch (kind) {
    case unary_expr::uo_plus:
      break;
    case unary_expr::uo_minus:
      operand_value = -operand_value;
      break;
    default:
      assert(false);
  }
  if (op_type.is(type::INTEGER) && check_double_to_int(operand_value)) {
    return typed_value(type(type::INTEGER),
                       static_cast<INTEGER_T>(operand_value), false);
  }
  return typed_value(type(type::FLOAT_POINT), operand_value, false);
}

std::optional<typed_value>
sema::_binary_on_tuple_num(const type& lhs_type, std::any lhs_value,
                           const type& rhs_type, std::any rhs_value,
                           std::size_t op_loc, std::function<_binary_op_impl_t> impl) const {
  assert(lhs_type.is(type::TUPLE) != rhs_type.is(type::TUPLE));
  std::vector<std::any> tuple;
  const type* tuple_type, * other_type;
  std::any other;
  if (lhs_type.is(type::TUPLE)) {
    tuple = std::any_cast<std::vector<std::any>>(std::move(lhs_value));
    other = std::move(rhs_value);
    tuple_type = &lhs_type;
    other_type = &rhs_type;
  } else {
    tuple = std::any_cast<std::vector<std::any>>(std::move(rhs_value));
    other = std::move(lhs_value);
    tuple_type = &rhs_type;
    other_type = &lhs_type;
  }
  std::vector<typed_value> _result_untidy;
  _result_untidy.reserve(tuple.size());
  for (auto& elem : tuple) {
    auto _op_result = impl(tuple_type->get_sub_type(), std::move(elem),
                           *other_type, other, op_loc);
    if (!_op_result) {
      return std::nullopt;
    }
    _result_untidy.emplace_back(std::move(*_op_result));
  }
  auto tidy_result = tidy_tuple(_result_untidy, {});
  assert(tidy_result);
  return tidy_result;
}

std::optional<typed_value>
sema::_unary_on_tuple_elem(const type& operand_type, std::any operand_value, std::size_t op_loc,
                           std::function<_unary_op_impl_t> impl) const {
  assert(operand_type.is(type::TUPLE));
  std::vector<std::any> tuple = std::any_cast<decltype(tuple)>(std::move(operand_value));
  std::vector<typed_value> _result_untidy;
  _result_untidy.reserve(tuple.size());
  for (auto& elem : tuple) {
    auto _op_result = impl(operand_type.get_sub_type(), std::move(elem), op_loc);
    if (!_op_result)
      return std::nullopt;
    _result_untidy.emplace_back(std::move(*_op_result));
  }
  auto tidy_result = tidy_tuple(_result_untidy, {});
  assert(tidy_result);
  return tidy_result;
}

#define BINARY_ON_TUPLE_NUM(CALLBACK)                                                   \
return _binary_on_tuple_num(lhs_type, std::move(lhs_value), rhs_type,                   \
                            std::move(rhs_value), op_loc,                               \
  [this](const type& lhs_type, std::any lhs,                                            \
         const type& rhs_type, std::any rhs,                                            \
         std::size_t op_loc) {                                                          \
           return CALLBACK(lhs_type, std::move(lhs),                                    \
                           rhs_type, std::move(rhs), op_loc);                           \
})

#define UNARY_ON_TUPLE_ELEM(CALLBACK)                                                   \
return _unary_on_tuple_elem(op_type, std::move(op_value), op_loc,                       \
  [this](const type& t, std::any v, std::size_t loc) {                                  \
    return _unary_minus_unchecked(t, std::move(v), loc);                                \
});

std::optional<typed_value>
sema::_add_unchecked(const type& lhs_type, std::any lhs_value,
                     const type& rhs_type, std::any rhs_value,
                     std::size_t op_loc) const {
  if (lhs_type.is_not(type::TUPLE) && rhs_type.is_not(type::TUPLE)) {
    // basic type
    return _binary_on_basic_type(lhs_type, std::move(lhs_value),
                                 rhs_type, std::move(rhs_value),
                                 op_loc, binary_expr::bo_add);
  } else if (lhs_type.is(type::TUPLE) && rhs_type.is(type::TUPLE)) {
    std::vector<std::any> _lhs_tuple = std::any_cast<std::vector<std::any>>(std::move(lhs_value));
    std::vector<std::any> _rhs_tuple = std::any_cast<std::vector<std::any>>(std::move(rhs_value));
    _lhs_tuple.insert(_lhs_tuple.end(), _rhs_tuple.begin(), _rhs_tuple.end());
    return typed_value(lhs_type, std::make_any<std::vector<std::any>>(std::move(_lhs_tuple)));
  } else {
    BINARY_ON_TUPLE_NUM(_add_unchecked);
  }
}

std::optional<typed_value>
sema::_sub_unchecked(const type& lhs_type, std::any lhs_value,
                     const type& rhs_type, std::any rhs_value,
                     std::size_t op_loc) const {
  if (lhs_type.is_not(type::TUPLE) && rhs_type.is_not(type::TUPLE)) {
    // basic type
    return _binary_on_basic_type(lhs_type, std::move(lhs_value),
                                 rhs_type, std::move(rhs_value),
                                 op_loc, binary_expr::bo_sub);
  } else {
    BINARY_ON_TUPLE_NUM(_sub_unchecked);
  }
}

std::optional<typed_value>
sema::_mul_unchecked(const type& lhs_type, std::any lhs_value, const type& rhs_type, std::any rhs_value,
                     std::size_t op_loc) const {
  if (lhs_type.is_not(type::TUPLE) && rhs_type.is_not(type::TUPLE)) {
    // basic type
    return _binary_on_basic_type(lhs_type, std::move(lhs_value),
                                 rhs_type, std::move(rhs_value),
                                 op_loc, binary_expr::bo_mul);
  } else {
    // multiply tuple element with number
    BINARY_ON_TUPLE_NUM(_mul_unchecked);
  }
}

std::optional<typed_value>
sema::_div_unchecked(const type& lhs_type, std::any lhs_value,
                     const type& rhs_type, std::any rhs_value,
                     std::size_t op_loc) const {
  if (lhs_type.is_not(type::TUPLE) && rhs_type.is_not(type::TUPLE)) {
    // basic type
    return _binary_on_basic_type(lhs_type, std::move(lhs_value),
                                 rhs_type, std::move(rhs_value),
                                 op_loc, binary_expr::bo_div);
  } else {
    BINARY_ON_TUPLE_NUM(_div_unchecked);
  }
}

std::optional<typed_value>
sema::_pow_unchecked(const type& lhs_type, std::any lhs_value,
                     const type& rhs_type, std::any rhs_value,
                     std::size_t op_loc) const {
  if (lhs_type.is_not(type::TUPLE) && rhs_type.is_not(type::TUPLE)) {
    // basic type
    return _binary_on_basic_type(lhs_type, std::move(lhs_value),
                                 rhs_type, std::move(rhs_value),
                                 op_loc, binary_expr::bo_pow);
  } else {
    BINARY_ON_TUPLE_NUM(_pow_unchecked);
  }
}

std::optional<typed_value>
sema::_unary_plus_unchecked(const type& op_type, std::any op_value,
                            std::size_t op_loc) const {
  return typed_value(op_type, std::move(op_value));
}

std::optional<typed_value>
sema::_unary_minus_unchecked(const type& op_type, std::any op_value,
                             std::size_t op_loc) const {
  if (op_type.is_not(type::TUPLE)) {
    // basic type
    return _unary_on_basic_type(op_type, std::move(op_value),
                                op_loc, unary_expr::uo_minus);
  } else {
    UNARY_ON_TUPLE_ELEM(_unary_minus_unchecked);
  }
}

int sema::_compare_basic_type(const type& lhs_type, std::any lhs_value,
                              const type& rhs_type, std::any rhs_value) const {
  if (lhs_type.is_not(type::STRING) && rhs_type.is_not(type::STRING)) {
    auto [l, r] = _extract_basic_integer_value(lhs_type, std::move(lhs_value),
                                               rhs_type, std::move(rhs_value));
    // FIXME: Is there a better way that does not cause overflow?
    if (l < r)
      return -1;
    if (l == r)
      return 0;
    return 1;
  }
  if (lhs_type.is(type::STRING) && rhs_type.is(type::STRING)) {
    auto [l, r] = _extract_basic_string_value(lhs_type, std::move(lhs_value),
                                              rhs_type, std::move(rhs_value));
    auto result = l.compare(r);
    if (result)
      return result / std::abs(result);
    return 0;
  }
  return -2;
}

int sema::compare(const type& lhs_type, std::any lhs_value,
                  const type& rhs_type, std::any rhs_value,
                  std::size_t op_loc) {
  // cannot compare anything with `VOID`
  if (lhs_type.is(type::VOID) || rhs_type.is(type::VOID)) {
    return -2;
  }
  if (lhs_type.is_not(type::TUPLE) && rhs_type.is_not(type::TUPLE)) {
    // basic type
    return _compare_basic_type(lhs_type, std::move(lhs_value),
                               rhs_type, std::move(rhs_value));
  }
  if (lhs_type.is(type::TUPLE) && rhs_type.is(type::TUPLE)) {
    using tuple_t = std::vector<std::any>;
    tuple_t lhs_tuple = std::any_cast<tuple_t>(std::move(lhs_value));
    tuple_t rhs_tuple = std::any_cast<tuple_t>(std::move(rhs_value));
    std::size_t size = std::min(lhs_tuple.size(), rhs_tuple.size());
    for (std::size_t i = 0; i < size; ++i) {
      auto elem_compare_result = compare(lhs_type.get_sub_type(), lhs_tuple[i],
                                         rhs_type.get_sub_type(), rhs_tuple[i],
                                         op_loc);
      if (elem_compare_result == -2)
        return elem_compare_result;
      // (lhs_elem == rhs_elem)
      if (elem_compare_result != 0) {
        return elem_compare_result;
      }
    }
    if (lhs_tuple.size() < rhs_tuple.size())
      return -1;
    if (lhs_tuple.size() == rhs_tuple.size())
      return 0;
    return 1;
  }
  return -2;
}

INTERPRETER_NAMESPACE_END
