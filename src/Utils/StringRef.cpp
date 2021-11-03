/**
 * This file provides implementation of @code{string_ref} interfaces.
 *
 * @author 19030500131 zy
 */
#include <Utils/StringRef.h>

INTERPRETER_NAMESPACE_BEGIN

static int _compare_same_length_str_insensitive(
    const char* lhs, const char* rhs, std::size_t length) {
  for (std::size_t idx = 0; idx < length; ++idx) {
    // note: before C++20, narrowing conversion to signed integer type
    // is implementation-defined.
    unsigned char _l = std::tolower(lhs[idx]);
    unsigned char _r = std::tolower(rhs[idx]);
    if (_l != _r)
      return _l < _r ? -1 : 1;
  }
  return 0;
}

int string_ref::compare_insensitive(string_ref rhs) const {
  if (int result =
      _compare_same_length_str_insensitive(_data, rhs._data, std::min(_length, rhs._length))) {
    return result;
  }
  if (_length != rhs._length)
    return _length < rhs._length ? -1 : 1;
  return 0;
}

INTERPRETER_NAMESPACE_END
