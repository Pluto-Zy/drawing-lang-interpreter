#include <Interpret/InternalSupport/InternalImpl.h>
#include <Sema/IdentifierInfo.h>
#include <cmath>

// OpenCV
#include <opencv2/imgproc.hpp>

INTERPRETER_NAMESPACE_BEGIN

void internal_impl::export_all_symbols(symbol_table& table) {
#define PREDEFINED_VARIABLE(NAME, TYPE, VALUE) \
  table.add_variable(token_kind::tk_identifier, #NAME, make_info_from_var(_##NAME));
#define PREDEFINED_VARIABLE_WITH_FILTER(NAME, TYPE, VALUE, VALUE_FILTER) \
  table.add_variable(token_kind::tk_identifier, #NAME,                   \
    make_info_from_var(_##NAME,                                          \
      [this](DIAG d, std::add_const_t<decltype(_##NAME)>& v) -> bool     \
        { return VALUE_FILTER(d, v); }));
#define PREDEFINED_CONSTANT(NAME, TYPE, VALUE) \
  table.add_variable(token_kind::tk_identifier, #NAME, make_info_from_constant(_##NAME));
#define PREDEFINED_FUNCTION(SPELLING, FUNC_NAME, RET, ...)        \
  table.add_function(token_kind::tk_identifier, #SPELLING,        \
      make_info_from_mem_func(this, &internal_impl::FUNC_NAME));
#define PREDEFINED_CONST_FUNCTION(SPELLING, FUNC_NAME, RET, ...)  \
  table.add_function(token_kind::tk_identifier, #SPELLING,        \
      make_info_from_mem_func(this, &internal_impl::FUNC_NAME));
#include "Interpret/InternalSupport/Predefined.h"
}

bool internal_impl::_origin_value_filter(diag_info_pack& pack,
                                         const std::vector<INTEGER_T>& value) const {
  assert(pack.param_loc.size() == 2);
  if (value.size() != 2) {
    pack.engine.create_diag(err_assign_elem_count, pack.param_loc[1])
      << "origin" << 2 << value.size() << diag_build_finish;
    return false;
  }
  return true;
}

bool internal_impl::_scale_value_filter(diag_info_pack& pack,
                                        const std::vector<FLOAT_POINT_T>& value) const {
  assert(pack.param_loc.size() == 2);
  if (value.size() != 2) {
    pack.engine.create_diag(err_assign_elem_count, pack.param_loc[1])
        << "scale" << 2 << value.size() << diag_build_finish;
    return false;
  }
  return true;
}

bool internal_impl::_background_size_value_filter(diag_info_pack& pack,
                                                  const std::vector<INTEGER_T>& value) const {
  assert(pack.param_loc.size() == 2);
  if (value.size() != 2) {
    pack.engine.create_diag(err_assign_elem_count, pack.param_loc[1])
        << "background_size" << 2 << value.size() << diag_build_finish;
    return false;
  }
  if (value[0] <= 0 || value[1] <= 0) {
    pack.engine.create_diag(err_size_value, pack.param_loc[1])
        << (value[0] <= 0 ? value[0] : value[1])
        << "background_size" << diag_build_finish;
    return false;
  }
  if (_have_drawn) {
    pack.engine.create_diag(warn_set_after_drawing, pack.param_loc[0])
        << "background_size" << diag_build_finish;
    return false;
  }
  return true;
}

bool internal_impl::_line_width_value_filter(diag_info_pack& pack,
                                             const INTEGER_T& value) const {
  if (value <= 0 || value > 10) {
    pack.engine.create_diag(err_line_width, pack.param_loc[1])
      << value << diag_build_finish;
    return false;
  }
  return true;
}

bool internal_impl::_background_color_value_filter(diag_info_pack& pack,
                                                   const std::vector<INTEGER_T>& value) const {
  if (value.size() != 3 && value.size() != 4) {
    pack.engine.create_diag(err_assign_elem_count, pack.param_loc[1])
        << "background_color" << "3 or 4" << value.size() << diag_build_finish;
    return false;
  }
  for (auto val : value) {
    if (val < 0 || val > 255) {
      pack.engine.create_diag(err_color_value, pack.param_loc[1])
          << val << diag_build_finish;
      return false;
    }
  }
  if (_have_drawn) {
    pack.engine.create_diag(warn_set_after_drawing, pack.param_loc[0])
        << "background_color" << diag_build_finish;
    return false;
  }
  return true;
}

bool internal_impl::_line_color_value_filter(diag_info_pack& pack,
                                             const std::vector<INTEGER_T>& value) const {
  if (value.size() != 3 && value.size() != 4) {
    pack.engine.create_diag(err_assign_elem_count, pack.param_loc[1])
        << "line_color" << "3 or 4" << value.size() << diag_build_finish;
    return false;
  }
  for (auto val : value) {
    if (val < 0 || val > 255) {
      pack.engine.create_diag(err_color_value, pack.param_loc[1])
          << val << diag_build_finish;
      return false;
    }
  }
  return true;
}

void internal_impl::_create_map() {
  _draw_map = cv::Mat(cv::Size(_background_size[0], _background_size[1]), CV_8UC3);
  _draw_map.setTo(cv::Scalar(_background_color[2], _background_color[1], _background_color[0]));
  _have_drawn = true;
}

cv::Point2d internal_impl::_transform(cv::Point2d input) const {
  FLOAT_POINT_T x = input.x;
  FLOAT_POINT_T y = input.y;
  x *= _scale[0];
  y *= _scale[1];
  auto cos = std::cos(_rot);
  auto sin = std::sin(_rot);
  FLOAT_POINT_T x_temp = x * cos + y * sin;
  y = y * cos - x * sin;
  x = x_temp;
  x += _origin[0];
  y += _origin[1];
  if (std::isinf(x) || x > std::numeric_limits<INTEGER_T>::max()) {

  }
  return {x, y};
}

void internal_impl::_draw_point(cv::Point2d p) {
  if (_draw_map.rows == 0 || _draw_map.cols == 0)
    _create_map();
  cv::Point real = _transform(p);
  if (real.x >= _draw_map.cols || real.x < 0 || real.y >= _draw_map.rows || real.y < 0)
    return;
  cv::circle(_draw_map, real, _line_width,
             cv::Scalar(_line_color[2], _line_color[1], _line_color[0]), cv::FILLED, cv::LINE_AA);
}

INTERPRETER_NAMESPACE_END
