/**
 * This file defines the @code{diag_data} structure.
 *
 * @code{diag_data} is used to pass diagnostic information.
 * Most diagnostic information will contain the line number
 * and column number, but not all information contains them.
 *
 * @author 19030500131 zy
 */

#ifndef DRAWING_LANG_INTERPRETER_DIAGDATA_H
#define DRAWING_LANG_INTERPRETER_DIAGDATA_H

#include <Utils/StringRef.h>
#include <filesystem>
#include <string>
#include <vector>

INTERPRETER_NAMESPACE_BEGIN

class diag_consumer;

struct fix_hint {
  /**
   * The range of the column of characters to be replaced.
   */
  std::pair<std::size_t, std::size_t> replace_range;
  /**
   * The actual code to insert at the location.
   */
  std::string code_to_insert;

  fix_hint() : replace_range(1, 0) { }

  [[nodiscard]] bool is_valid() const {
    return replace_range.first <= replace_range.second;
  }
};

struct diag_data {
  // basic information of diagnostic data
  /**
   * Severity.
   */
  enum { ERROR, WARNING, NOTE } level;
  /**
   * The corresponding line in the source file.
   */
  string_ref source_line;
  /**
   * The name of the source file.
   */
  std::basic_string<std::filesystem::path::value_type> file_name;
  /**
   * The line number of the location to report.
   */
  std::size_t line_idx;
  /**
   * The column number of the starting position of the range to be reported.
   */
  std::size_t column_start_idx;
  /**
   * The column number of the ending position of the range to be reported.
   */
  std::size_t column_end_idx;
  /**
   * Original diagnostic information.
   */
  string_ref origin_diag_message;
  /**
   * Represents whether the current object is valid.
   */
  bool is_invalid;

  /**
   * Saves the params used to replace the placeholders.
   */
  std::vector<std::string> _params;
  /**
   * Diagnostic information after parameter replacement.
   */
  std::string _result_diag_message;

  /**
   * provides a hint with some code to insert or modify
   * at a particular position.
   */
  fix_hint fix;
  /**
   * Consumer for reporting information.
   */
  diag_consumer* consumer;

  bool has_file_name() const { return !file_name.empty(); }
  bool has_line() const { return !source_line.empty(); }
  bool has_column() const { return column_start_idx < column_end_idx; }
  bool is_column_range() const { return has_column() && column_end_idx - column_start_idx > 1; }
  bool has_fix_hint() const { return fix.is_valid(); }
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_DIAGDATA_H
