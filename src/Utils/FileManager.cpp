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

std::error_code file_manager::_read_file_and_set_to_buf(const std::filesystem::path& file_path,
                                                        std::uintmax_t file_size) {
  // FIXME: The code can be better and more efficient.
  std::unique_ptr<char[]> _temp_buf(static_cast<char*>(operator new[](file_size + 1, std::nothrow)));
  if (!_temp_buf)
    return std::make_error_code(std::errc::not_enough_memory);
  std::ifstream fin(file_path);
  if (!fin.is_open())
    return std::make_error_code(std::errc::invalid_argument);
  _file_name = file_path;
  _length = fin.read(_temp_buf.get(), file_size).gcount();
  fin.close();
  // if the end of the file is not '\n', add it
  if (!_length || _temp_buf[_length - 1] != '\n')
    _temp_buf[_length++] = '\n';
  _data_buf = std::move(_temp_buf);
  return std::make_error_code(std::errc());
}

std::error_code file_manager::from_file(const std::filesystem::path& file_path) {
  std::error_code result_ec = is_valid_file(file_path);
  if (result_ec)
    return result_ec;
  auto file_size = fs::file_size(file_path, result_ec);
  if (result_ec)
    return result_ec;
  return _read_file_and_set_to_buf(file_path, file_size);
}
INTERPRETER_NAMESPACE_END
