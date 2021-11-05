#include <Utils/FileManager.h>
#include <fstream>
#include <iterator>
#include <algorithm>

INTERPRETER_NAMESPACE_BEGIN

namespace fs = ::std::filesystem;

/**
 * Check whether the file exists and whether the file is a regular file.
 */
static std::error_code is_valid_file(const std::filesystem::path &file_path) {
  std::error_code ec;
  fs::is_regular_file(file_path, ec);
  return ec;
}

bool file_manager::_reset_to_new_buf(std::size_t file_size) {
  std::size_t _buf_size = _file_name_length + file_size;
  auto _temp_buf = static_cast<char*>(operator new[](_buf_size, std::nothrow));
  if (!_temp_buf)
    return false;
  _data_buf.reset(_temp_buf);
  _length = _buf_size;
  return true;
}

std::error_code file_manager::_read_file_and_set_to_buf(const std::filesystem::path& file_path) {
  // FIXME: The code can be better and more efficient.
  std::ifstream fin(file_path);
  if (!fin.is_open())
    return std::make_error_code(std::errc::invalid_argument);
  // copy the file name to the start of the buffer

  // we cannot use std::copy_n(file_path.string().begin(), file_path.string().end(), ...)
  // because it will return two different strings so that `begin()` and `end()` will point
  // to different arrays.
  auto ptr = std::copy_n(file_path.c_str(), file_path.string().size(),
            _data_buf.get());
  *ptr = '\0';  // set null terminator
  fin.read(const_cast<char*>(get_file_buf_begin()), get_file_buf_end() - get_file_buf_begin());
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
