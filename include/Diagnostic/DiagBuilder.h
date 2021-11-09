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

struct diag_build_finish_t { };
/**
 * Marks the end position of the parameter sequence.
 */
constexpr diag_build_finish_t diag_build_finish;

class diag_builder {
public:
  diag_builder() = delete;
  ~diag_builder();

  [[nodiscard]] const diag_data& get_diag_data() const { return *_internal_data; }

  const diag_builder& arg(string_ref argument) const;
  const diag_builder& arg(std::int64_t value) const;

  /**
   * Replaces all replaceable placeholders with
   * corresponding parameters.
   */
  const diag_builder& replace_all_arg() const;

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
  explicit diag_builder(diag_data* data) : _internal_data(data) { }
};

inline const diag_builder& operator<<(const diag_builder& lhs, string_ref rhs) {
  return lhs.arg(rhs);
}

inline const diag_builder& operator<<(const diag_builder& lhs, std::int64_t rhs) {
  return lhs.arg(rhs);
}

inline const diag_builder& operator<<(const diag_builder& lhs, diag_build_finish_t) {
  lhs.replace_all_arg();
  lhs.report_to_consumer();
  return lhs;
}

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_DIAGBUILDER_H
