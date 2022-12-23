/**
 * This file defines the @code{typed_value} class.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_TYPEDVALUE_H
#define DRAWING_LANG_INTERPRETER_TYPEDVALUE_H

#include <AST/Type.h>
#include <any>

INTERPRETER_NAMESPACE_BEGIN

/**
 * This class represents a type-value pair.
 */
class typed_value {
public:
  typed_value(type t, std::any v, bool constant = false)
    : _type(std::move(t)), value(std::move(v)),
    _is_constant(constant) { }
  typed_value(const typed_value& other) = default;
  typed_value& operator=(const typed_value& other) = default;
  typed_value(typed_value&&) = default;
  typed_value& operator=(typed_value&&) = default;
  [[nodiscard]] const type& get_type() const { return _type; }
  [[nodiscard]] type&& take_type() && { return std::move(_type); }
  [[nodiscard]] std::any get_value() const { return value; }
  // TODO: Change the signature of the function as take_type
  [[nodiscard]] std::any take_value() { return std::move(value); }
  [[nodiscard]] bool is_constant() const { return _is_constant; }
  void set_type(type t) { _type = std::move(t); }
  void set_value(std::any v) { value = std::move(v); }
  void make_constant() { _is_constant = true; }

  [[nodiscard]] std::string get_value_spelling() const;
private:
  type _type;
  std::any value;
  bool _is_constant;

  static std::string _get_value_spelling_impl(const type& value_type, std::any value);
};

/**
 * Returns the spelling of the value in specific forms.
 */
inline std::string typed_value::get_value_spelling() const {
  return _get_value_spelling_impl(_type, value);
}

inline std::string typed_value::_get_value_spelling_impl(const type& value_type, std::any value) {
  switch (value_type.get_kind()) {
    case type::INTEGER:
      return std::to_string(unpack_value<INTEGER_T>(std::move(value)));
    case type::FLOAT_POINT:
      return std::to_string(unpack_value<FLOAT_POINT_T>(std::move(value)));
    case type::STRING:
      return unpack_value<STRING_T>(std::move(value));
    case type::TUPLE: {
      std::string result = "(";
      std::vector<std::any> elems = std::any_cast<std::vector<std::any>>(value);
      for (std::size_t i = 0; i < elems.size(); ++i) {
        result += (i ? ", " : "") +
            _get_value_spelling_impl(value_type.get_sub_type(), std::move(elems[i]));
      }
      result += ")";
      return result;
    }
    default:
      assert(false);
      return "unknown value";
  }
}

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_TYPEDVALUE_H
