/**
 * This is a similar implementation of LLVM @code{StringRef}.
 *
 * @code{string_ref} represents a constant reference to a string,
 * which is implemented with a pointer and an integer.
 *
 * In the implementation of LLVM, @code{StringRef} has many string
 * operations suitable for the compilation process. Here I first
 * provide basic operations. If the subsequent development process
 * requires more operations, continue to add.
 *
 * @note The class does not own the data of the string.
 *
 * @author 19030500131 zy
 */

#ifndef DRAWING_LANG_INTERPRETER_STRINGREF_H
#define DRAWING_LANG_INTERPRETER_STRINGREF_H

#include "def.h"
#include <cstring>
#include <string>

INTERPRETER_NAMESPACE_BEGIN

class string_ref {
public:
  static constexpr std::size_t npos = static_cast<std::size_t>(-1);

  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  using value_type      = char;         // only represents a string composed of char
  using iterator        = const char*;  // cannot change the string from a StringRef
  using const_iterator  = const char*;
private:
  const value_type* _data;
  size_type _length;

  // a constexpr version of std::strlen
  static constexpr size_type _str_len(const char* str) {
    const char* ptr = str;
    for (; *ptr != '\0'; ++ptr)
      ;
    return static_cast<size_type>(ptr - str);
  }

  // compares two raw string with the same length
  static constexpr int
  _compare_same_length_str(const char* lhs, const char* rhs, size_type length) {
    for (std::size_t idx = 0; idx < length; ++idx) {
      if (lhs[idx] != rhs[idx])
        return lhs[idx] - rhs[idx];
    }
    return 0;
  }
public:
  // constructors
  constexpr string_ref() : _data(nullptr), _length(0) { }
  constexpr string_ref(const value_type* str)
    : _data(str), _length(str ? _str_len(str) : 0) { }
  constexpr string_ref(const value_type* str, size_type length)
    : _data(str), _length(length) { }
  string_ref(const std::string& str)
    : _data(str.data()), _length(str.size()) { }
  constexpr string_ref(std::nullptr_t) = delete;  // cannot initialize from a null pointer
  string_ref(std::string&&) = delete;             // cannot initialize from a temp string

  // iterators
  constexpr iterator begin() const { return _data; }
  constexpr iterator end() const { return _data + _length; }

  // basic operations of a string
  constexpr const char* data() const { return _data; }
  std::string str() const { return _data ? std::string(_data, _length) :
                                           std::string(); }
  constexpr bool empty() const { return _length == 0; }
  constexpr size_type size() const { return _length; }

  constexpr char front() const {
    // do not check whether it is empty
    return _data[0];
  }

  constexpr char back() const {
    // do not check whether it is empty
    return _data[_length - 1];
  }

  constexpr bool equals(string_ref rhs) const {
    return _length == rhs._length &&
      _compare_same_length_str(_data, rhs._data, _length) == 0;
  }

  /**
   * If @code{*this} is less than @code{rhs} lexicographically, returns a negative number;
   * if it is greater than @code{rhs}, returns a positive number;
   * otherwise, returns @code{0}.
   */
  constexpr int compare(string_ref rhs) const {
    if (int result =
        _compare_same_length_str(_data, rhs._data, std::min(_length, rhs._length))) {
      return result;
    }
    if (_length != rhs._length)
      return _length < rhs._length ? -1 : 1;
    return 0;
  }

  /**
   * compares @code{*this} with @code{rhs} and ignores case.
   */
  int compare_insensitive(string_ref rhs) const;

  // operator overloads
  constexpr char operator[](size_type idx) const {
    return _data[idx];
  }

  // If we simply delete operator=(std::string&&),
  // we cannot use string_ref = {} or string_ref = "literal".
  template<class T, std::enable_if_t<std::is_same_v<T, std::string>, int> = 0>
  string_ref& operator=(T&&) = delete;

  explicit operator std::string() const { return str(); }
};

constexpr bool operator==(string_ref lhs, string_ref rhs) {
  return lhs.equals(rhs);
}
constexpr bool operator!=(string_ref lhs, string_ref rhs) {
  return !(lhs == rhs);
}
constexpr bool operator<(string_ref lhs, string_ref rhs) {
  return lhs.compare(rhs) < 0;
}
constexpr bool operator>(string_ref lhs, string_ref rhs) {
  return rhs < lhs;
}
constexpr bool operator<=(string_ref lhs, string_ref rhs) {
  return !(rhs < lhs);
}
constexpr bool operator>=(string_ref lhs, string_ref rhs) {
  return !(lhs < rhs);
}

std::size_t hash_value(string_ref s);
std::size_t hash_value_lower_case(string_ref s);

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_STRINGREF_H
