#include <Utils/FileManager.h>
#include <fstream>
#include <iterator>

INTERPRETER_NAMESPACE_BEGIN

namespace fs = ::std::filesystem;

/**
 * Check whether the file exists and whether the file is a regular file.
 */
static std::error_code is_valid_file(const std::filesystem::path &file_path) {
  std::error_code ec;
  bool result = fs::is_regular_file(file_path, ec);
  if (!result)
    return std::make_error_code(std::errc::invalid_argument);
  return ec;
}

bool file_manager::_reset_to_new_buf(std::size_t file_size) {
  auto _temp_buf = static_cast<char*>(operator new[](file_size, std::nothrow));
  if (!_temp_buf)
    return false;
  _data_buf.reset(_temp_buf);
  _length = file_size;
  return true;
}

std::error_code file_manager::_read_file_and_set_to_buf(const std::filesystem::path& file_path) {
  // FIXME: The code can be better and more efficient.
  std::ifstream fin(file_path);
  if (!fin.is_open())
    return std::make_error_code(std::errc::invalid_argument);
  _file_name = file_path;
  fin.read(_data_buf.get(), _length);
  // FIXME: Consider more about this code.
  for (; _data_buf[_length - 1] < 0; --_length)
    ;
  fin.close();
  return std::make_error_code(std::errc());
}

std::error_code file_manager::from_file(const std::filesystem::path& file_path) {
  std::error_code result_ec = is_valid_file(file_path);
  if (result_ec)
    return result_ec;
  auto file_size = fs::file_size(file_path, result_ec);
  if (result_ec)
    return result_ec;
  if (!_reset_to_new_buf(file_size))
    return std::make_error_code(std::errc::not_enough_memory);
  return _read_file_and_set_to_buf(file_path);
}
INTERPRETER_NAMESPACE_END
