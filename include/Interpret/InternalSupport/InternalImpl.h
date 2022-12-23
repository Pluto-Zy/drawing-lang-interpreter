/**
 * This file provides the definition of internal variables,
 * constants and functions.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_INTERNALIMPL_H
#define DRAWING_LANG_INTERPRETER_INTERNALIMPL_H

#include <AST/Type.h>
#include <type_traits>
#include <functional>

// OpenCV
#include <opencv2/core.hpp>

INTERPRETER_NAMESPACE_BEGIN

class symbol_table;
struct diag_info_pack;

/**
 * This class contains all the predefined symbols (including variable,
 * constant and function), which are listed in @file{Predefined.h}.
 */
class internal_impl {
public:
  void export_all_symbols(symbol_table& table);
private:
#define PREDEFINED_VARIABLE_WITH_FILTER(NAME, TYPE, VALUE, FILTER) PREDEFINED_VARIABLE(NAME, TYPE, VALUE)
#define PREDEFINED_VARIABLE(NAME, TYPE, VALUE) TYPE _##NAME = VALUE;
#define PREDEFINED_CONSTANT(NAME, TYPE, VALUE) std::add_const_t<TYPE> _##NAME = TYPE(VALUE);
#define PREDEFINED_FUNCTION(spelling, FUNC_NAME, RET, ...) RET FUNC_NAME(__VA_ARGS__);
#define PREDEFINED_CONST_FUNCTION(spelling, FUNC_NAME, RET, ...) RET FUNC_NAME(__VA_ARGS__) const;
#define REGISTER_VALUE_FILTER(VAR_NAME, FUNC_NAME) \
  bool FUNC_NAME(DIAG, std::add_const_t<decltype(_##VAR_NAME)>&) const;
#include "Predefined.h"

  // internal status
  bool _have_drawn = false;
  cv::Mat _draw_map;
  void _create_map();
  cv::Point2d _transform(cv::Point2d input) const;
  void _draw_point(cv::Point2d p);
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_INTERNALIMPL_H
