/**
 * This file provides implementation of @code{diag_consumer} interfaces.
 *
 * @author 19030500131 zy
 */
#include "Diagnostic/DiagConsumer.h"
#include <Diagnostic/DiagData.h>
#include <iostream>

INTERPRETER_NAMESPACE_BEGIN

template<class Ty>
void print_file_name(const Ty& str);

template<>
void print_file_name<std::string>(const std::string& str) {
  std::cerr << str;
}
template<>
void print_file_name<std::wstring>(const std::wstring& str) {
  std::wcerr << str;
}

void cmd_diag_consumer::report(const drawing::diag_data* data) {
  if (data->has_file_name()) {
    print_file_name(data->file_name);
    std::cerr << ':';
  }
  if (data->has_line())
    std::cerr << data->line_idx + 1 << ':';
  if (data->has_fix_hint())
    std::cerr << data->fix.replace_range.first << ": ";
  else if (data->has_column())
    std::cerr << data->column_start_idx << ": ";
  switch (data->level) {
    case diag_data::ERROR:
      std::cerr << "error: ";
      break;
    case diag_data::WARNING:
      std::cerr << "warning: ";
      break;
    case diag_data::NOTE:
      std::cerr << "note: ";
      break;
  }
  std::cerr << data->_result_diag_message << '\n';
  if (data->has_line()) {
    std::cerr << static_cast<std::string>(data->source_line) << '\n';
    if (data->has_fix_hint()) {
      auto& hint = data->fix;
      std::cerr << std::string(hint.replace_range.first, ' ') << '^';
      for (auto i = hint.replace_range.first + 1; i < hint.replace_range.second; ++i)
        std::cerr << '~';
      std::cerr << '\n';
      std::cerr << std::string(hint.replace_range.first, ' ') << hint.code_to_insert;
    } else if (data->has_column()) {
      std::cerr << std::string(data->column_start_idx, ' ') << '^';
      if (data->is_column_range()) {
        for (auto i = data->column_start_idx + 1; i < data->column_end_idx; ++i)
          std::cerr << '~';
      }
    }
    std::cerr << '\n';
  }
}

INTERPRETER_NAMESPACE_END
