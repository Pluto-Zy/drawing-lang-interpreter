#include <Interpret/InternalSupport/InternalImpl.h>
#include <Sema/IdentifierInfo.h>
#include <iostream>
#include <random>
#include <algorithm>

// opencv
#include <opencv2/imgcodecs.hpp>

INTERPRETER_NAMESPACE_BEGIN

VOID_T
internal_impl::_internal_print_integer(INTEGER_T arg1) const {
  std::cout << "print: " << arg1 << '\n';
}

VOID_T
internal_impl::_internal_print_double(FLOAT_POINT_T arg1) const {
  std::cout << "print: " << arg1 << '\n';
}

VOID_T
internal_impl::_internal_print_string(STRING_T arg1) const {
  std::cout << "print: " << arg1 << '\n';
}

VOID_T
internal_impl::_internal_print_integer_tuple(std::vector<INTEGER_T> arg1) const {
  std::cout << "print: (";
  for (std::size_t i = 0; i < arg1.size(); ++i) {
    std::cout << (i ? ", " : "") << arg1[i];
  }
  std::cout << ")\n";
}

VOID_T
internal_impl::_internal_print_float_tuple(std::vector<FLOAT_POINT_T> arg1) const {
  std::cout << "print: (";
  for (std::size_t i = 0; i < arg1.size(); ++i) {
    std::cout << (i ? ", " : "") << arg1[i];
  }
  std::cout << ")\n";
}

std::vector<INTEGER_T>
internal_impl::_internal_str_to_color(DIAG d, STRING_T arg1) const {
  static std::unordered_map<STRING_T, STRING_T> predefined_colors = {
      {"red", "#FF0000"},
      {"green", "#00FF00"},
      {"blue", "#0000FF"},
  };
  std::vector<INTEGER_T> result(3);
  if (auto iter = predefined_colors.find(arg1); iter != predefined_colors.end()) {
    arg1 = iter->second;
  }
  if (arg1.size() != 7)
    goto invalid_color;
  if (arg1[0] != '#')
    goto invalid_color;
  for (std::size_t i = 1; i < 7; ++i) {
    auto ch = std::tolower(arg1[i]);
    if (!(std::isdigit(ch) || (ch >= 'a' && ch <= 'f')))
      goto invalid_color;
  }
  result[0] = std::stoi(arg1.substr(1, 2), nullptr, 16);
  result[1] = std::stoi(arg1.substr(3, 2), nullptr, 16);
  result[2] = std::stoi(arg1.substr(5, 2), nullptr, 16);
  if (std::any_of(result.begin(), result.end(), [](INTEGER_T val) { return val < 0 || val > 255; }))
    goto invalid_color;
  return result;
invalid_color:
  d.engine.create_diag(err_color_str, d.param_loc[0], d.param_loc[1]) << arg1 << diag_build_finish;
  d.success = false;
  return {};
}

INTEGER_T
internal_impl::_internal_abs_integer(DIAG d, INTEGER_T arg1) const {
  if (arg1 == std::numeric_limits<INTEGER_T>::min()) {
    d.engine.create_diag(err_param_value, d.param_loc[0], d.param_loc[1])
      << -static_cast<long long>(arg1) << "abs" << diag_build_finish;
    d.success = false;
    return 0;
  }
  return -arg1;
}

FLOAT_POINT_T
internal_impl::_internal_abs_float(FLOAT_POINT_T arg1) const {
  return -arg1;
}

FLOAT_POINT_T
internal_impl::_internal_cos_float(FLOAT_POINT_T arg1) const {
  return std::cos(arg1);
}

FLOAT_POINT_T
internal_impl::_internal_sin_float(FLOAT_POINT_T arg1) const {
  return std::sin(arg1);
}

FLOAT_POINT_T
internal_impl::_internal_tan_float(DIAG d, FLOAT_POINT_T arg1) const {
  auto result = std::tan(arg1);
  if (std::isnan(result) || std::isinf(result)) {
    d.engine.create_diag(err_param_value, d.param_loc[0], d.param_loc[1])
      << arg1 << "tan" << diag_build_finish;
    d.success = false;
  }
  return result;
}

FLOAT_POINT_T
internal_impl::_internal_ln_float(DIAG d, FLOAT_POINT_T arg1) const {
  auto result = std::log(arg1);
  if (std::isnan(result) || std::isinf(result)) {
    d.engine.create_diag(err_param_value, d.param_loc[0], d.param_loc[1])
        << arg1 << "ln" << diag_build_finish;
    d.success = false;
  }
  return result;
}


INTEGER_T
internal_impl::_internal_rand_integer(INTEGER_T arg1, INTEGER_T arg2) const {
  std::random_device r;
  std::default_random_engine e(r());
  std::uniform_int_distribution<INTEGER_T> uniform(arg1, arg2);
  return uniform(e);
}

VOID_T
internal_impl::_internal_draw_xy(DIAG d, FLOAT_POINT_T arg1, FLOAT_POINT_T arg2) {
  _draw_point(cv::Point2f(arg1, arg2));
}

VOID_T
internal_impl::_internal_overload_integer(INTEGER_T arg1, INTEGER_T arg2) const {
  std::cout << "call overload function for integer\n";
}

VOID_T
internal_impl::_internal_overload_float(FLOAT_POINT_T arg1, FLOAT_POINT_T arg2) const {
  std::cout << "call overload function for float_point\n";
}

VOID_T
internal_impl::_internal_save_img(STRING_T path) {
  if (!_have_drawn) {
    _create_map();
  }
  cv::Mat dst;
  cv::flip(_draw_map, dst, 0);
  cv::imwrite(path, dst);
}

INTERPRETER_NAMESPACE_END
