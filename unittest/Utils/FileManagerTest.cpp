#include <Utils/FileManager.h>
#include <fstream>
#include <gtest/gtest.h>

INTERPRETER_NAMESPACE_BEGIN

namespace fs = std::filesystem;

static std::error_code create_temp_file(const std::string& temp_file_name,
                                        fs::path& result) {
  std::error_code result_ec;
  auto temp_path = fs::temp_directory_path(result_ec);
  if (result_ec)
    return result_ec;
  fs::path _temp_file_path = temp_path / temp_file_name;
  std::ofstream _temp_out(_temp_file_path, std::ios::trunc);
  _temp_out.close();
  result = _temp_file_path;
  return result_ec;
}

TEST(file_manager_test, read_file) {
  fs::path temp_file_path;
  std::error_code ec = create_temp_file("drawing_file_manager_test", temp_file_path);
  ASSERT_FALSE(ec);
  std::ofstream out(temp_file_path);
  ASSERT_TRUE(out.is_open());
  std::string input_data = "some data\nand a new line\tand some spaces\n";
  out << input_data;
  out.close();
  // start test
  {
    file_manager manager;
    EXPECT_FALSE(manager.from_file(temp_file_path));
    EXPECT_FALSE(manager.is_invalid());
    EXPECT_EQ(manager.file_size(), input_data.size());
    EXPECT_EQ(manager.file_size(), manager.get_file_buf_end() - manager.get_file_buf_begin());
    EXPECT_EQ(manager.get_file_name(), string_ref(temp_file_path.c_str()));
    for (auto iter = manager.get_file_buf_begin(), src = input_data.c_str();
        iter != manager.get_file_buf_end(); ++iter, ++src) {
      EXPECT_EQ(*iter, *src);
    }
  }
  ASSERT_TRUE(fs::remove(temp_file_path));
  {
    file_manager manager;
    EXPECT_TRUE(manager.from_file(temp_file_path));
    EXPECT_TRUE(manager.is_invalid());
  }
}

INTERPRETER_NAMESPACE_END
