/**
 * This file defines the interface of @code{diag_engine} and some helper enumerators.
 *
 * @code{diag_engine} is a class used to generate a diagnostic message. There are 3
 * kinds of diagnosis: ERROR, WARNING and NOTE. All types of diagnostic information
 * that can be reported are defined in the @file{DiagTypes.h} file.
 *
 * @code{diag_engine} needs the type of information to be reported, and the offset
 * (or pair of offsets) of location in the source code. It will calculate the real
 * line number and column number according to the offset(s).
 *
 * @code{diag_engine} uses a pointer @code{file_manager} to manage the content of
 * the file.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_DIAGENGINE_H
#define DRAWING_LANG_INTERPRETER_DIAGENGINE_H

#include "Utils/def.h"
#include "DiagBuilder.h"

#include <vector>
#include <optional>

INTERPRETER_NAMESPACE_BEGIN

class file_manager;
class diag_consumer;
class string_ref;
struct diag_data;

enum error_types {
#define ERROR(err_type, err_str) err_type,
#include "DiagTypes.h"
#undef ERROR
};

enum warning_types {
#define WARNING(warning_type, warning_str) warning_type,
#include "DiagTypes.h"
#undef WARNING
};

enum note_types {
#define NOTE(note_type, note_str) note_type,
#include "DiagTypes.h"
#undef NOTE
};

class diag_engine {
public:
  diag_engine() : _file_manager(nullptr), _diag_consumer(nullptr) { }
  void set_file(const file_manager* manager);
  void set_consumer(diag_consumer* consumer);

  [[nodiscard]] diag_builder create_diag(error_types diag_type) const;

  [[nodiscard]] diag_builder create_diag(error_types diag_type,
                                         std::size_t location) const;

  [[nodiscard]] diag_builder create_diag(error_types diag_type,
                                         std::size_t start_loc,
                                         std::size_t end_loc) const;

  [[nodiscard]] diag_builder create_diag(warning_types diag_type) const;

  [[nodiscard]] diag_builder create_diag(warning_types diag_type,
                                         std::size_t location) const;

  [[nodiscard]] diag_builder create_diag(warning_types diag_type,
                                         std::size_t start_loc,
                                         std::size_t end_loc) const;

  [[nodiscard]] diag_builder create_diag(note_types diag_type) const;

  [[nodiscard]] diag_builder create_diag(note_types diag_type,
                                         std::size_t location) const;

  [[nodiscard]] diag_builder create_diag(note_types diag_type,
                                         std::size_t start_loc,
                                         std::size_t end_loc) const;
private:
  const file_manager* _file_manager;
  diag_consumer* _diag_consumer;
  /**
   * Saves the start location of each line.
   */
  std::vector<std::size_t> _lines;

  void _generate_line_cache();

  [[nodiscard]] std::optional<std::size_t>
  _get_line_num(std::size_t location, bool& invalid) const;

  [[nodiscard]] string_ref
  _get_source_line(std::size_t line_idx) const;

  [[nodiscard]] diag_data*
  _create_diag_impl(string_ref diag_msg,
                    std::size_t location_begin,
                    std::size_t location_end) const;
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_DIAGENGINE_H
