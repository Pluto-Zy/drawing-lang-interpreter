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

INTERPRETER_NAMESPACE_BEGIN

class file_manager {
private:
  /**
   * The length of the buffer used to store the
   * file name at the beginning of the buffer.
   */
  constexpr static std::size_t _file_name_length = 256;
  /**
   * The buffer used to save the contents of the file.
   */
  std::unique_ptr<char[]> _data_buf;
  /**
   * The length of the data buffer.
   */
  std::size_t _length;

  /**
   * Calculates the real length of the buffer according to @code{file_size}
   * and allocates an uninitialized buffer, then save the pointer in the
   * smart pointer.
   * @param file_size The length of the file content.
   * @return Returns @code{false} if there is no enough memory to save the buffer,
   *         otherwise returns @code{true}.
   */
  [[nodiscard]] bool _reset_to_new_buf(std::size_t file_size);
  /**
   * Reads the content of the file. Save the file name
   * and file content to the buffer.
   */
  [[nodiscard]] std::error_code _read_file_and_set_to_buf(const std::filesystem::path& file_path);
public:
  file_manager() = default;

  /**
   * @return Returns a pointer to the starting position
   * of the buffer that holds the contents of the file.
   */
  const char* get_file_buf_begin() const { return &_data_buf[_file_name_length]; }
  const char* get_file_buf_end() const { return _data_buf.get() + _length; }
  [[nodiscard]] string_ref get_file_name() const {
    return {_data_buf.get()};
  }
  bool is_invalid() const { return _length < _file_name_length; }
  std::size_t file_size() const { return _length - _file_name_length; }

  [[nodiscard]] std::error_code from_file(const std::filesystem::path& file_path);
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_FILEMANAGER_H
