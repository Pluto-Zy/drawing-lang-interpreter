/**
 * This file provides implementation of @code{diag_engine} interfaces
 * and defines the prompt text corresponding to the diagnosis type.
 *
 * @author 19030500131 zy
 */
#include <Diagnostic/DiagEngine.h>
#include <Diagnostic/DiagData.h>
#include <Utils/FileManager.h>
#include <algorithm>

INTERPRETER_NAMESPACE_BEGIN

std::vector<std::tuple<string_ref, decltype(diag_data::level)>> _diag_info = {
#define ERROR(err_type, err_str) { err_str, diag_data::ERROR },
#define WARNING(warning_type, warning_str) { warning_str, diag_data::WARNING },
#define NOTE(note_type, note_str) { note_str, diag_data::NOTE },
#include "Diagnostic/DiagTypes.h"
#undef ERROR
#undef WARNING
#undef NOTE
};

void diag_engine::set_file(const file_manager *manager) {
  _file_manager = manager;
  _generate_line_cache();
}


void diag_engine::set_consumer(diag_consumer *consumer) {
  _diag_consumer = consumer;
}

void diag_engine::_generate_line_cache() {
  if (!_file_manager)
    return;
  decltype(_lines) _temp_result;
  auto beg = _file_manager->get_file_buf_begin(), end = _file_manager->get_file_buf_end();
  for (auto ch = beg, line_beg = beg; ch != end; ++ch) {
    if (*ch == '\n') {
      _temp_result.push_back(static_cast<decltype(_lines)::value_type>(line_beg - beg));
      line_beg = ch + 1;
    }
  }
  _temp_result.push_back(_file_manager->file_size());
  _lines = std::move(_temp_result);
}

std::optional<std::size_t>
diag_engine::_get_line_num(std::size_t location, bool& invalid) const {
  if (!_file_manager || _lines.size() < 2)
    return std::nullopt;
  if (location >= _file_manager->file_size()) {
    invalid = true;
    return std::nullopt;
  }
  auto iter = std::upper_bound(_lines.begin(), _lines.end(), location);
  return static_cast<std::size_t>(iter - _lines.begin() - 1);
}

string_ref diag_engine::_get_source_line(std::size_t line_idx) const {
  if (!_file_manager || _file_manager->file_size() == 0)
    return {};
  std::size_t length = _lines[line_idx + 1] - _lines[line_idx];
  const char* beg = _file_manager->get_file_buf_begin() + _lines[line_idx];
  // length cannot be 0 here
  if (beg[length - 1] == '\n')
    --length;
  return {beg, length};
}

diag_data* diag_engine::_create_diag_impl(string_ref diag_msg,
                                          std::size_t location_begin,
                                          std::size_t location_end) const {
  // FIXME: Process bad alloc here?
  auto result = new diag_data;
  result->consumer = _diag_consumer;
  // find the line number of the location
  bool invalid = false;
  if (location_begin <= location_end) {
    auto begin_line_opt = _get_line_num(location_begin, invalid);
    if (begin_line_opt) {
      result->line_idx = *begin_line_opt;
      result->source_line = _get_source_line(*begin_line_opt);
      // set the column
      result->column_start_idx = location_begin - _lines[*begin_line_opt];
      result->column_end_idx = location_end - _lines[*begin_line_opt];
      if (result->column_end_idx > result->source_line.size() + 1)
        invalid = true;
    }
  }
  // set the file name
  if (_file_manager)
    result->file_name = _file_manager->get_file_name();
  result->origin_diag_message = diag_msg;
  result->is_invalid = invalid;
  return result;
}

diag_builder diag_engine::create_diag(diag_id diag_type) const {
  return create_diag(diag_type, 1, 0);
}

diag_builder diag_engine::create_diag(diag_id diag_type,
                                      std::size_t location) const {
  return create_diag(diag_type, location, location + 1);
}

diag_builder diag_engine::create_diag(diag_id diag_type,
                                      std::size_t start_loc,
                                      std::size_t end_loc) const {
  diag_data* result = _create_diag_impl(std::get<0>(_diag_info[diag_type]), start_loc, end_loc);
  result->level = std::get<1>(_diag_info[diag_type]);
  return static_cast<diag_builder>(result);
}

fix_hint diag_engine::create_insertion_after_location(std::size_t location,
                                                      string_ref code) const {
  fix_hint result;
  bool invalid = false;
  auto line = _get_line_num(location, invalid);
  if (!line)
    return result;
  std::size_t column = location - _lines[*line];
  result.replace_range = {column + 1, column + 2};
  result.code_to_insert = static_cast<std::string>(code);
  return result;
}

fix_hint diag_engine::create_replacement(std::size_t beg, std::size_t end,
                                         string_ref code) const {
  fix_hint result;
  bool invalid = false;
  auto line = _get_line_num(beg, invalid);
  if (!line)
    return result;
  result.replace_range.first = beg - _lines[*line];
  result.replace_range.second = end - _lines[*line];
  result.code_to_insert = static_cast<std::string>(code);
  return result;
}

INTERPRETER_NAMESPACE_END
