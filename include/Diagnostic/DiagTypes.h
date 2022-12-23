/**
 * This file defines all diagnostic information that can be reported.
 *
 * @author 1903050131 zy
 */
#ifndef ERROR
#define ERROR(err_type, err_str)
#endif
#ifndef WARNING
#define WARNING(warning_type, warning_str)
#endif
#ifndef NOTE
#define NOTE(note_type, note_str)
#endif

// Used for Test
ERROR(err_test_type, "This is a test error message.")
WARNING(warn_test_type, "This is a test warning message.")
NOTE(note_test_type, "This is a test note message.")
ERROR(err_test_with_param_type, "This is a test error message with param: %0.")

// Start
ERROR(err_no_input_file, "no input file")
ERROR(err_open_file, "cannot open file '%0'")

// Lexer
WARNING(null_in_file, "null character ignored")
ERROR(err_unknown_char, "invalid character '%0'")
WARNING(warn_miss_str_terminate, "missing terminating '\"' character")

// Parser
ERROR(err_expect, "expected '%0'")
ERROR(err_expect_after, "expected '%0' after '%1'")
ERROR(err_expect_semi_after, "expected ';' after %0")
ERROR(err_expect_expr, "expected expression")
NOTE(note_match_l_paren, "to match this '('")
NOTE(note_match_l_brace, "to match this '{'")
ERROR(err_constant_too_large, "constant literal is too large to be represented in a double type")
ERROR(err_cannot_be_unary, "'%0' cannot be a unary operator")
ERROR(err_expect_variable, "expected variable")
WARNING(warn_unknown_escape, "unknown escape sequence '%0'")

// Sema
ERROR(err_use_func_as_var, "use function as a variable")
ERROR(err_use_unknown_identifier, "use of unknown identifier")
ERROR(err_use_unknown_identifier_with_hint, "use of unknown identifier; did you mean '%0'?")
ERROR(err_conflict_tuple_elem_type, "deduced conflicting types ('%0' vs '%1') for tuple element type")
ERROR(err_use_var_as_func, "called object is not a function")
NOTE(note_candidate_func_param_count_mismatch,
     "candidate function not viable: requires %0 argument(s), but %1 was provided")
NOTE(note_candidate_func_param_type_mismatch,
     "candidate function not viable: no known conversion from '%0' to '%1' for %2 argument")
ERROR(err_no_match_func, "no matching function for call to '%0'")
ERROR(err_ambiguous_call, "call of overloaded '%0' is ambiguous")
NOTE(note_candidate, "candidate: '%0'")
WARNING(warn_narrow_conversion, "conversion from '%0' to '%1' changes value from '%2' to '%3'")
ERROR(err_invalid_binary_operand, "invalid operands to binary expression ('%0' and '%1')")
ERROR(err_invalid_binary_result, "invalid result of %0 '%1' and '%2'")
WARNING(warn_div_zero, "division by zero")
ERROR(err_invalid_unary_operand, "invalid operand to unary expression ('%0')")
ERROR(err_mul_str_negative_num, "cannot multiply a string with a negative number '%0'")

// Interpreter
ERROR(err_assign_constant, "cannot assign to constant")
ERROR(err_assign_elem_count, "invalid value for '%0': requires %1 argument(s), but %2 provided")
ERROR(err_size_value, "invalid value '%0' for '%1': cannot use negative numbers or zeros as size")
ERROR(err_color_value, "invalid value '%0' used as color: the value must be between 0 and 255")
ERROR(err_line_width, "invalid value '%0' for 'line_width'")
WARNING(warn_set_after_drawing, "setting '%0' after drawing: value ignored")
ERROR(err_assign_incompatible_type, "assigning to '%0' from incompatible type '%1'")
ERROR(err_invalid_compare_type, "cannot compare '%0' with '%1'")
ERROR(err_deduced_variable_type, "cannot define variable of type '%0'")

// Internal Impl
ERROR(err_color_str, "invalid color value '%0'")
ERROR(err_param_value, "invalid value '%0' for '%1'")

#undef ERROR
#undef WARNING
#undef NOTE
