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

#undef ERROR
#undef WARNING
#undef NOTE
