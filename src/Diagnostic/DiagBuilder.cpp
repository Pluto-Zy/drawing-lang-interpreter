/**
 * This file provides implementation of @code{diag_builder} interfaces.
 *
 * @author 19030500131 zy
 */
#include <Diagnostic/DiagBuilder.h>
#include <Diagnostic/DiagData.h>
#include <Diagnostic/DiagConsumer.h>
#include <iostream>

INTERPRETER_NAMESPACE_BEGIN

const diag_builder& diag_builder::arg(string_ref argument) const {
  _internal_data->_params.push_back(static_cast<std::string>(argument));
  return *this;
}

const diag_builder& diag_builder::arg(std::int64_t value) const {
  _internal_data->_params.push_back(std::to_string(value));
  return *this;
}

const diag_builder& diag_builder::arg(char ch) const {
  _internal_data->_params.push_back({ch});
  return *this;
}

const diag_builder& diag_builder::arg(fix_hint hint) const {
  _internal_data->fix = std::move(hint);
  return *this;
}

std::string diag_builder::_process_escape_char(char ch) const {
  // support '%' + digit and '%%' currently
  if (std::isdigit(ch)) {
    int idx = ch - '0';
    if (static_cast<std::size_t>(idx) < _internal_data->_params.size())
      return _internal_data->_params[idx];
  }
  if (ch == '%')
    return "%";
  return {'%', ch};
}

const diag_builder& diag_builder::replace_all_arg() const {
  std::string result;
  string_ref origin_ref = _internal_data->origin_diag_message;
  result.reserve(origin_ref.size());
  for (auto iter = origin_ref.begin(); iter != origin_ref.end(); ++iter) {
    if (*iter != '%')
      result += *iter;
    else {
      // process escape character
      ++iter;
      if (iter != origin_ref.end())
        result += _process_escape_char(*iter);
      else {
        result += '%';
        break;
      }
    }
  }
  _internal_data->_result_diag_message = std::move(result);
  return *this;
}

void diag_builder::report_to_consumer() const {
  if (_internal_data->consumer) {
    _internal_data->consumer->report(_internal_data.get());
  }
}

diag_builder::diag_builder(diag_data* data) : _internal_data(data) { }

diag_builder& diag_builder::operator=(diag_builder&&) noexcept = default;
diag_builder::diag_builder(diag_builder&&) noexcept = default;
diag_builder::~diag_builder() = default;

const diag_builder& operator<<(const diag_builder& lhs, fix_hint hint) {
  lhs.arg(std::move(hint));
  return lhs;
}

INTERPRETER_NAMESPACE_END
