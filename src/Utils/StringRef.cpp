/**
 * This file provides implementation of @code{string_ref} interfaces.
 *
 * @author 19030500131 zy
 */
#include <Utils/StringRef.h>
#include <functional>
#include <memory>

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

unsigned string_ref::edit_distance(string_ref rhs, bool ignore_cases,
                                   bool allow_replacements, unsigned int max_distance) const {
  /*
   * The same implementation of llvm::edit_distance.
   * The algorithm is described at http://en.wikipedia.org/wiki/Levenshtein_distance.
   */
  size_type m = size();
  size_type n = rhs.size();

  constexpr unsigned small_buffer_size = 64;
  unsigned small_buffer[small_buffer_size];
  std::unique_ptr<unsigned[]> allocated;
  unsigned* row = small_buffer;
  if (n + 1 > small_buffer_size) {
    row = new unsigned[n + 1];
    allocated.reset(row);
  }

  for (unsigned i = 1; i <= n; ++i)
    row[i] = i;

  for (size_type y = 1; y <= m; ++y) {
    row[0] = y;
    unsigned best_this_row = row[0];
    char cur_ch = static_cast<char>(ignore_cases ?
        std::tolower((*this)[y - 1]) : (*this)[y - 1]);

    unsigned previous = y - 1;
    for (size_type x = 1; x <= n; ++x) {
      int old_row = row[x];
      char cur_rhs_ch = static_cast<char>(ignore_cases ?
          std::tolower(rhs[x - 1]) : rhs[x - 1]);
      if (allow_replacements) {
        row[x] = std::min(previous + (cur_ch == cur_rhs_ch ? 0u : 1u),
                          std::min(row[x - 1], row[x]) + 1);
      } else {
        if (cur_ch == cur_rhs_ch)
          row[x] = previous;
        else
          row[x] = std::min(row[x - 1], row[x]) + 1;
      }
      previous = old_row;
      best_this_row = std::min(best_this_row, row[x]);
    }

    if (max_distance && best_this_row > max_distance)
      return max_distance + 1;
  }

  return row[n];
}

std::size_t hash_value(string_ref s) {
  std::size_t seed = 0;
  for (char ch : s) {
    seed ^= std::hash<string_ref::value_type>()(ch) + 0x9e3779b9
        + (seed << 6) + (seed >> 2);
  }
  return seed;
}

std::size_t hash_value_lower_case(string_ref s) {
  std::size_t seed = 0;
  for (char ch : s) {
    seed ^= std::hash<string_ref::value_type>()(static_cast<char>(std::tolower(ch)))
        + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
}

INTERPRETER_NAMESPACE_END
