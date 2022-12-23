/**
 * This file lists all the predefined symbols (including variables,
 * constants and functions).
 *
 * PREDEFINED_VARIABLE - Defines a predefined variable, whose
 * name is @code{NAME}, type is @code{TYPE} and initial value is
 * @code{INITIAL_VALUE}. Because it is not convenient to use commas
 * in macro definitions, if you need to use a list as the initial
 * value, you should use the @code{LIST} macro.
 *
 * PREDEFINED_VARIABLE_WITH_FILTER - Defines a predefined variable
 * and adds a value filter (see below) to it.
 *
 * PREDEFINED_CONSTANT - Defines a predefined constant. Constants
 * cannot be modified or assigned at runtime.
 *
 * PREDEFINED_FUNCTION - Defines a predefined function. The real
 * name of the function is @code{FUNC_NAME}. The name used when
 * calling it in the language is @code{NAME}.
 *
 * PREDEFINED_CONST_FUNCTION - Defines a predefined const
 * member function.
 *
 * REGISTER_VALUE_FILTER - Defines a value filter used for
 * @code{VAR_NAME}, whose name is @code{FUNC_NAME}.
 * A value filter is a function. When trying to modify the
 * value of a variable, the value is first passed to the v
 * alue filter (if any). All value filters have the following
 * form:
 *
 *   bool FUNC_NAME(diag_pack_info&, const VAR_TYPE&)
 *
 * where the @code{VAR_TYPE} is the type of the corresponding
 * variable to which the filter is attached. You can check
 * the value first in the value filter and if the value is
 * invalid, you can make a diagnostic message by the
 * @code{diag_pack_info}. If the value filter returns @code{false},
 * the value will not be assigned to the variable.
 *
 * @author 19030500131 zy
 */
#ifndef PREDEFINED_VARIABLE
#define PREDEFINED_VARIABLE(NAME, TYPE, INITIAL_VALUE)
#endif
#ifndef PREDEFINED_VARIABLE_WITH_FILTER
#define PREDEFINED_VARIABLE_WITH_FILTER(NAME, TYPE, INITIAL_VALUE, VALUE_FILTER)
#endif
#ifndef PREDEFINED_CONSTANT
#define PREDEFINED_CONSTANT(NAME, TYPE, VALUE)
#endif
#ifndef PREDEFINED_FUNCTION
#define PREDEFINED_FUNCTION(SPELLING, FUNC_NAME, RET, ...)
#endif
#ifndef PREDEFINED_CONST_FUNCTION
#define PREDEFINED_CONST_FUNCTION(SPELLING, FUNC_NAME, RET, ...)
#endif
#ifndef REGISTER_VALUE_FILTER
#define REGISTER_VALUE_FILTER(VAR_NAME, FUNC_NAME)
#endif

#define LIST(...) { __VA_ARGS__ }
#define DIAG diag_info_pack&

PREDEFINED_VARIABLE_WITH_FILTER(origin, std::vector<INTEGER_T>, LIST(0, 0), _origin_value_filter)
PREDEFINED_VARIABLE(rot, FLOAT_POINT_T, 0.0)
PREDEFINED_VARIABLE_WITH_FILTER(scale, std::vector<FLOAT_POINT_T>, LIST(1, 1), _scale_value_filter)
PREDEFINED_VARIABLE(t, FLOAT_POINT_T, 0.0)
PREDEFINED_VARIABLE(P, std::vector<FLOAT_POINT_T>, LIST(0))
PREDEFINED_VARIABLE_WITH_FILTER(background_size, std::vector<INTEGER_T>, LIST(500, 500), _background_size_value_filter)
PREDEFINED_VARIABLE_WITH_FILTER(background_color, std::vector<INTEGER_T>, LIST(255, 255, 255), _background_color_value_filter)
PREDEFINED_VARIABLE_WITH_FILTER(line_width, INTEGER_T, 1, _line_width_value_filter)
PREDEFINED_VARIABLE_WITH_FILTER(line_color, std::vector<INTEGER_T>, LIST(0, 0, 0), _line_color_value_filter)

PREDEFINED_CONSTANT(PI, FLOAT_POINT_T, 3.14159265358979323846)
PREDEFINED_CONSTANT(E, FLOAT_POINT_T, 2.718281828459)

PREDEFINED_CONST_FUNCTION(print, _internal_print_integer, VOID_T, INTEGER_T)
PREDEFINED_CONST_FUNCTION(print, _internal_print_double, VOID_T, FLOAT_POINT_T)
PREDEFINED_CONST_FUNCTION(print, _internal_print_string, VOID_T, STRING_T)
PREDEFINED_CONST_FUNCTION(print, _internal_print_integer_tuple, VOID_T, std::vector<INTEGER_T>)
PREDEFINED_CONST_FUNCTION(print, _internal_print_float_tuple, VOID_T, std::vector<FLOAT_POINT_T>)
PREDEFINED_CONST_FUNCTION(color, _internal_str_to_color, std::vector<INTEGER_T>, DIAG, STRING_T)
// math function
PREDEFINED_CONST_FUNCTION(abs, _internal_abs_integer, INTEGER_T, DIAG, INTEGER_T)
PREDEFINED_CONST_FUNCTION(abs, _internal_abs_float, FLOAT_POINT_T, FLOAT_POINT_T)
PREDEFINED_CONST_FUNCTION(cos, _internal_cos_float, FLOAT_POINT_T, FLOAT_POINT_T)
PREDEFINED_CONST_FUNCTION(sin, _internal_sin_float, FLOAT_POINT_T, FLOAT_POINT_T)
PREDEFINED_CONST_FUNCTION(tan, _internal_tan_float, FLOAT_POINT_T, DIAG, FLOAT_POINT_T)
PREDEFINED_CONST_FUNCTION(ln, _internal_ln_float, FLOAT_POINT_T, DIAG, FLOAT_POINT_T)
PREDEFINED_CONST_FUNCTION(rand_int, _internal_rand_integer, INTEGER_T, INTEGER_T, INTEGER_T)
// draw function
PREDEFINED_FUNCTION(draw, _internal_draw_xy, VOID_T, DIAG, FLOAT_POINT_T, FLOAT_POINT_T)
PREDEFINED_FUNCTION(save, _internal_save_img, VOID_T, STRING_T)
// used for overload
PREDEFINED_CONST_FUNCTION(overload_func, _internal_overload_integer, VOID_T, INTEGER_T, INTEGER_T)
PREDEFINED_CONST_FUNCTION(overload_func, _internal_overload_float, VOID_T, FLOAT_POINT_T, FLOAT_POINT_T)

REGISTER_VALUE_FILTER(origin, _origin_value_filter)
REGISTER_VALUE_FILTER(scale, _scale_value_filter)
REGISTER_VALUE_FILTER(background_size, _background_size_value_filter)
REGISTER_VALUE_FILTER(line_width, _line_width_value_filter)
REGISTER_VALUE_FILTER(background_color, _background_color_value_filter)
REGISTER_VALUE_FILTER(line_color, _line_color_value_filter)

#undef PREDEFINED_VARIABLE
#undef PREDEFINED_VARIABLE_WITH_FILTER
#undef PREDEFINED_CONSTANT
#undef PREDEFINED_FUNCTION
#undef PREDEFINED_CONST_FUNCTION
#undef REGISTER_VALUE_FILTER
