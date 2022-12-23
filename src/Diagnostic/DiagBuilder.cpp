/**
 * This file provides implementation of @code{diag_builder} interfaces.
 *
 * @author 19030500131 zy
 */
#include <Diagnostic/DiagBuilder.h>
#include <Diagnostic/DiagData.h>
#include <Diagnostic/DiagConsumer.h>
#include <sstream>

INTERPRETER_NAMESPACE_BEGIN

void diag_builder::arg(std::string argument) const {
  _internal_data->_params.emplace_back(std::move(argument));
}

void diag_builder::arg(std::int64_t value) const {
  _internal_data->_params.push_back(std::to_string(value));
}

void diag_builder::arg(double value) const {
  std::stringstream s;
  s.precision(15);
  s << value;
  _internal_data->_params.emplace_back(s.str());
}

void diag_builder::arg(char ch) const {
  _internal_data->_params.push_back({ch});
}

void diag_builder::arg(fix_hint hint) const {
  _internal_data->fix = std::move(hint);
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

void diag_builder::replace_all_arg() const {
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

diag_builder operator<<(diag_builder lhs, fix_hint hint) {
  lhs.arg(std::move(hint));
  return lhs;
}

INTERPRETER_NAMESPACE_END
