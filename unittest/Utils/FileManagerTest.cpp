#include <Utils/FileManager.h>
#include <fstream>
#include <gtest/gtest.h>

INTERPRETER_NAMESPACE_BEGIN

namespace fs = std::filesystem;

class file_manager_factory {
public:
  std::unique_ptr<file_manager> get_file_manager_from_temp_file(const std::string& input_data);
  ~file_manager_factory();
private:
  std::vector<fs::path> _created;
};

file_manager_factory::~file_manager_factory() {
  for (const auto& path : _created) {
    (void)fs::remove(path);
  }
}

template<class char_type>
std::basic_string<char_type> generate_temp_file_name();

template<>
std::string generate_temp_file_name() {
  static int index = 0;
  return "drawing_file_manager_test" + std::to_string(index++);
}

template<>
std::wstring generate_temp_file_name() {
  static int index = 0;
  return L"drawing_file_manager_test" + std::to_wstring(index++);
}

static std::error_code create_temp_file(const fs::path& temp_file_name,
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

std::unique_ptr<file_manager> file_manager_factory::get_file_manager_from_temp_file(
    const std::string& input_data) {
  fs::path temp_file_path;
  std::error_code ec =
      create_temp_file(generate_temp_file_name<fs::path::value_type>(), temp_file_path);
  if (ec)
    return nullptr;
  _created.push_back(temp_file_path);
  std::ofstream out(temp_file_path);
  if (!out.is_open())
    return nullptr;
  out << input_data;
  out.close();
  std::unique_ptr<file_manager> ptr = std::make_unique<file_manager>();
  ec = ptr->from_file(temp_file_path);
  if (ec)
    return nullptr;
  return ptr;
}

TEST(file_manager_test, invalid_file_type) {
  std::error_code ec;
  auto temp_path = fs::temp_directory_path(ec);
  ASSERT_FALSE(ec);
  file_manager manager;
  ec = manager.from_file(temp_path);
  EXPECT_EQ(ec, std::make_error_code(std::errc::invalid_argument));
  EXPECT_TRUE(manager.is_invalid());
}

TEST(file_manager_test, read_file) {
  {
    fs::path temp_file_path;
    std::error_code ec =
        create_temp_file(generate_temp_file_name<fs::path::value_type>(), temp_file_path);
    ASSERT_FALSE(ec);
    std::ofstream out(temp_file_path);
    ASSERT_TRUE(out.is_open());
    std::string input_data = "some data";
    out << input_data;
    out.close();

    file_manager manager;
    EXPECT_FALSE(manager.from_file(temp_file_path));
    EXPECT_FALSE(manager.is_invalid());
    EXPECT_EQ(manager.file_size(), input_data.size());
    EXPECT_EQ(manager.file_size(), manager.get_file_buf_end() - manager.get_file_buf_begin());
    EXPECT_EQ(manager.get_file_name(), temp_file_path);
    for (auto iter = manager.get_file_buf_begin(), src = input_data.c_str();
        iter != manager.get_file_buf_end(); ++iter, ++src) {
      EXPECT_EQ(*iter, *src);
    }
  }

  file_manager_factory factory;
  std::string datas[] = {
      "some data", "\n", "\r\n", "\r", "\n\r", "\r\r\n", "\r\n\r", "data\ndata2",
      "data\r\ndata2", "data\ndata2\r", "data\rdata2\n", "data\ndata2\r\n"
  };
  for (std::size_t i = 0; i < std::size(datas); ++i) {
    auto ptr = factory.get_file_manager_from_temp_file(datas[i]);
    ASSERT_TRUE(ptr);
    EXPECT_EQ(ptr->file_size(), datas[i].size()) << "input data idx: " << i;
    EXPECT_EQ(string_ref(ptr->get_file_buf_begin(), ptr->file_size()), string_ref(datas[i]));
  }
}

INTERPRETER_NAMESPACE_END
