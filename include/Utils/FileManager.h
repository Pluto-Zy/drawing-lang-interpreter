/**
 * This file defines the @code{file_manager} class.
 *
 * @code{file_manager} represents a single source file. It will read
 * the contents of the file and save it in a buffer. The lifetime
 * of the buffer is managed by a smart pointer.
 *
 * I did not consider the alignment of the buffer, because it might
 * make my program too complicated.
 *
 * The compiler should support the @code{filesystem} library
 * introduced in C++17.
 *
 * @author 19030500131 zy
 */

#ifndef DRAWING_LANG_INTERPRETER_FILEMANAGER_H
#define DRAWING_LANG_INTERPRETER_FILEMANAGER_H

#include "StringRef.h"
#include <filesystem>
#include <memory>
#include <utility>

INTERPRETER_NAMESPACE_BEGIN

class file_manager {
private:
  /**
   * The buffer used to save the contents of the file.
   */
  std::unique_ptr<const char[]> _data_buf;
  /**
   * The length of the data buffer.
   */
  std::size_t _length;

  using _file_name_t = std::basic_string<std::filesystem::path::value_type>;
  _file_name_t _file_name;
  /**
   * Reads the content of the file. Save the file content to the buffer.
   * This method will allocate enough memory to save the content.
   */
  [[nodiscard]] std::error_code _read_file_and_set_to_buf(const std::filesystem::path& file_path,
                                                          std::uintmax_t file_size);
public:
  file_manager() : _data_buf(nullptr), _length(0), _file_name() { }

  /**
   * @return Returns a pointer to the starting position
   * of the buffer that holds the contents of the file.
   */
  const char* get_file_buf_begin() const { return _data_buf.get(); }
  const char* get_file_buf_end() const { return _data_buf.get() + _length; }
  [[nodiscard]] const auto& get_file_name() const { return _file_name; }
  bool is_invalid() const { return !_data_buf; }
  std::size_t file_size() const { return _length; }

  [[nodiscard]] std::error_code from_file(const std::filesystem::path& file_path);
protected:
  // only used for unit test
  file_manager(const char* buf, std::size_t length, _file_name_t file_name) :
    _data_buf(buf), _length(length), _file_name(std::move(file_name)) { }
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_FILEMANAGER_H
