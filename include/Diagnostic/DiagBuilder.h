/**
 * This file defines @code{diag_builder} class.
 *
 * @code{diag_builder} is used to build a diagnostic message. It replaces
 * the placeholders(% + digit) in the original information with the parameters
 * provided by the user.
 *
 * For example,
 *      origin      param       result
 *      "%1 %0"     "a" "b"     "b a"
 *      "%0 %%"     2           "2 %"
 *      "%0 %2"     1 2 "a"     "1 a"
 *
 * The accepted parameter types are @code{string_ref} and @code{int64_t}.
 *
 * After the build is complete, @code{diag_builder} will provide the result
 * to @code{diag_consumer}, which is stored in @code{diag_data}.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_DIAGBUILDER_H
#define DRAWING_LANG_INTERPRETER_DIAGBUILDER_H

#include <Utils/StringRef.h>
#include <memory>

INTERPRETER_NAMESPACE_BEGIN

struct diag_data;
struct fix_hint;

struct diag_build_finish_t { };
/**
 * Marks the end position of the parameter sequence.
 */
constexpr diag_build_finish_t diag_build_finish;

class diag_builder {
public:
  diag_builder() = delete;
  diag_builder(const diag_builder&) = delete;
  diag_builder(diag_builder&&) noexcept;
  diag_builder& operator=(const diag_builder&) = delete;
  diag_builder& operator=(diag_builder&&) noexcept;
  ~diag_builder();

  [[nodiscard]] const diag_data& get_diag_data() const { return *_internal_data; }

  void arg(std::string argument) const;
  void arg(std::int64_t value) const;
  void arg(double value) const;
  void arg(char ch) const;
  void arg(fix_hint hint) const;

  /**
   * Replaces all replaceable placeholders with
   * corresponding parameters.
   */
  void replace_all_arg() const;

  /**
   * Provides the constructed @code{diag_data}
   * to @code{diag_consumer}.
   */
  void report_to_consumer() const;
private:
  /**
   * The @code{diag_builder} will manage the
   * lifetime of @code{diag_data} object.
   */
  std::unique_ptr<diag_data> _internal_data;
  friend class diag_engine;

  std::string _process_escape_char(char ch) const;
protected:
  explicit diag_builder(diag_data* data);
};

inline diag_builder operator<<(diag_builder lhs, string_ref rhs) {
  lhs.arg(static_cast<std::string>(rhs));
  return lhs;
}

inline diag_builder operator<<(diag_builder lhs, std::string rhs) {
  lhs.arg(std::move(rhs));
  return lhs;
}

inline diag_builder operator<<(diag_builder lhs, const char* rhs) {
  lhs.arg(static_cast<std::string>(rhs));
  return lhs;
}

template<class RT, std::enable_if_t<std::is_integral_v<RT> && !std::is_same_v<RT, char>, int> = 0>
inline diag_builder operator<<(diag_builder lhs, RT rhs) {
  lhs.arg(static_cast<std::int64_t>(rhs));
  return lhs;
}

inline diag_builder operator<<(diag_builder lhs, double rhs) {
  lhs.arg(rhs);
  return lhs;
}

inline diag_builder operator<<(diag_builder lhs, char rhs) {
  lhs.arg(rhs);
  return lhs;
}

inline diag_builder operator<<(diag_builder lhs, diag_build_finish_t) {
  lhs.replace_all_arg();
  lhs.report_to_consumer();
  return lhs;
}

diag_builder operator<<(diag_builder lhs, fix_hint hint);

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_DIAGBUILDER_H
