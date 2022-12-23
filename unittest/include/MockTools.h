#ifndef DRAWING_LANG_INTERPRETER_PARSETOOLS_H
#define DRAWING_LANG_INTERPRETER_PARSETOOLS_H

#include <Parse/Parser.h>
#include <Diagnostic/DiagConsumer.h>
#include <Diagnostic/DiagData.h>
#include <gtest/gtest.h>

#include <memory>

INTERPRETER_NAMESPACE_BEGIN

template<class str_type>
str_type generate_temp_file_name();

template<>
inline std::string generate_temp_file_name() {
  return "temp_lex_file";
}

template<>
inline std::wstring generate_temp_file_name() {
  return L"temp_lex_file";;
}

namespace {
class test_file_manager : public file_manager {
public:
  template<std::size_t N>
  explicit test_file_manager(const char(& str)[N]) :
      file_manager(copy(str), N /* with new line character */,
                   generate_temp_file_name<
                       std::remove_cv_t<
                           std::remove_reference_t<
                               decltype(std::declval<file_manager>().get_file_name())>>>()) {}

  template<std::size_t N>
  [[nodiscard]] char* copy(const char(& str)[N]) const {
    static_assert(N > 1, "cannot use zero-length string");
    assert(str[N - 2] != '\n');
    char* result = new char[N];
    std::memcpy(result, str, N - 1);
    result[N - 1] = '\n';
    return result;
  }
};

class test_diag_consumer : public diag_consumer {
public:
  void report(const diag_data* data) override {
    this->data.push_back(*data);
  }

  [[nodiscard]] const diag_data& get_data(std::size_t idx = 0) const {
    return data[idx];
  }

  [[nodiscard]] std::size_t get_data_size() const {
    return data.size();
  }

  void clear() { data.clear(); }

private:
  std::vector<diag_data> data;
};

} // namespace

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_PARSETOOLS_H
