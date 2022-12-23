/**
 * This file defines all the type in the language
 * with its real type and spelling.
 */
#ifndef BASIC_TYPE
#define BASIC_TYPE(front_end_type_name, actual_type, spelling)
#endif
#ifndef VARIABLE_BASIC_TYPE
#define VARIABLE_BASIC_TYPE(front_end_type_name, actual_type, spelling) \
  BASIC_TYPE(front_end_type_name, actual_type, spelling)
#endif

BASIC_TYPE(VOID, void, "Void")
VARIABLE_BASIC_TYPE(INTEGER, int, "Integer")
VARIABLE_BASIC_TYPE(FLOAT_POINT, double, "Double")
VARIABLE_BASIC_TYPE(STRING, std::string, "String")

#undef BASIC_TYPE
#undef VARIABLE_BASIC_TYPE
