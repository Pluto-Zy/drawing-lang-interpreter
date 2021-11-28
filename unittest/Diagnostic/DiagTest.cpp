#include <Diagnostic/DiagData.h>
#include <Diagnostic/DiagEngine.h>
#include <Diagnostic/DiagConsumer.h>
#include <Utils/FileManager.h>
#include <gtest/gtest.h>

INTERPRETER_NAMESPACE_BEGIN

using path_char_t = std::filesystem::path::value_type;

template<class char_type>
std::basic_string<char_type> get_temp_file_name();

template<>
std::string get_temp_file_name() {
  return "drawing_file_manager_test";
}

template<>
std::wstring get_temp_file_name() {
  return L"drawing_file_manager_test";
}

class temp_file_manager : public file_manager {
public:
  template<std::size_t N>
  temp_file_manager(const char(&str)[N]) :
    file_manager(copy(str), N - 1, get_temp_file_name<path_char_t>()) { }

  template<std::size_t N>
  [[nodiscard]] char* copy(const char(&str)[N]) const {
    static_assert(N > 0, "cannot use zero-length string");
    char* result = new char[N - 1];
    std::memcpy(result, str, N - 1);
    return result;
  }
};

class test_diag_consumer : public diag_consumer {
public:
  void report(const diag_data *data) override {
    this->data = *data;
  }
  const diag_data& get_data() const {
    return data;
  }
private:
  diag_data data;
};

class test_diag_builder : public diag_builder {
public:
  explicit test_diag_builder(diag_data* data) : diag_builder(data) { }
  static test_diag_builder create(string_ref origin, diag_consumer* consumer) {
    diag_data* obj = new diag_data;
    obj->origin_diag_message = origin;
    obj->consumer = consumer;
    return static_cast<test_diag_builder>(obj);
  }
};

TEST(diag_engine_test, basic) {
  diag_engine engine;
  {
    diag_builder result = engine.create_diag(err_test_type, 0);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.level, diag_data::ERROR);
    EXPECT_FALSE(data.has_file_name());
    EXPECT_FALSE(data.has_line());
    EXPECT_FALSE(data.has_column());
    EXPECT_FALSE(data.is_column_range());
    EXPECT_FALSE(data.is_invalid);
  }
  {
    temp_file_manager manager("some data");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 0);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.level, diag_data::ERROR);
    EXPECT_EQ(data.source_line, "some data");
    EXPECT_EQ(data.file_name, get_temp_file_name<path_char_t>());
    EXPECT_EQ(data.line_idx, 0);
    EXPECT_EQ(data.column_start_idx, 0);
    EXPECT_EQ(data.column_end_idx, 1);
    EXPECT_EQ(data.origin_diag_message, "This is a test error message.");
    EXPECT_FALSE(data.is_invalid);
    EXPECT_TRUE(data.has_file_name());
    EXPECT_TRUE(data.has_line());
    EXPECT_TRUE(data.has_column());
    EXPECT_FALSE(data.is_column_range());
  }
}

TEST(diag_engine_test, location) {
  diag_engine engine;
  {
    temp_file_manager manager("some data");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(warn_test_type, 4);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.level, diag_data::WARNING);
    EXPECT_EQ(data.source_line, "some data");
    EXPECT_EQ(data.line_idx, 0);
    EXPECT_EQ(data.column_start_idx, 4);
    EXPECT_EQ(data.column_end_idx, 5);
    EXPECT_EQ(data.origin_diag_message, "This is a test warning message.");
    EXPECT_TRUE(data.has_file_name());
    EXPECT_TRUE(data.has_line());
    EXPECT_TRUE(data.has_column());
    EXPECT_FALSE(data.is_column_range());
  }
  {
    temp_file_manager manager("a\nand a new line");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(note_test_type, 4);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.level, diag_data::NOTE);
    EXPECT_EQ(data.source_line, "and a new line");
    EXPECT_EQ(data.line_idx, 1);
    EXPECT_EQ(data.column_start_idx, 2);
    EXPECT_EQ(data.column_end_idx, 3);
    EXPECT_EQ(data.origin_diag_message, "This is a test note message.");
    EXPECT_TRUE(data.has_file_name());
    EXPECT_TRUE(data.has_line());
    EXPECT_TRUE(data.has_column());
    EXPECT_FALSE(data.is_column_range());
  }
  {
    temp_file_manager manager("a\nand a new line");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 0);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.source_line, "a");
    EXPECT_EQ(data.line_idx, 0);
    EXPECT_EQ(data.column_start_idx, 0);
    EXPECT_EQ(data.column_end_idx, 1);
  }
  {
    temp_file_manager manager("a\nand a new line");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 3, 3);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.source_line, "and a new line");
    EXPECT_EQ(data.line_idx, 1);
    EXPECT_EQ(data.column_start_idx, 1);
    EXPECT_EQ(data.column_end_idx, 1);
    EXPECT_FALSE(data.is_invalid);
    EXPECT_TRUE(data.has_line());
    EXPECT_FALSE(data.has_column());
  }
  {
    temp_file_manager manager("a\nb\n");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 0, 1);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.source_line, "a");
    EXPECT_EQ(data.line_idx, 0);
    EXPECT_EQ(data.column_start_idx, 0);
    EXPECT_EQ(data.column_end_idx, 1);
    EXPECT_FALSE(data.is_invalid);
  }
  {
    temp_file_manager manager("a\nb\n");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 0, 2);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.source_line, "a");
    EXPECT_EQ(data.line_idx, 0);
    EXPECT_EQ(data.column_start_idx, 0);
    EXPECT_EQ(data.column_end_idx, 2);
    EXPECT_TRUE(data.is_invalid);
  }
  {
    temp_file_manager manager("a\nb\n");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 1, 2);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.source_line, "a");
    EXPECT_EQ(data.line_idx, 0);
    EXPECT_EQ(data.column_start_idx, 1);
    EXPECT_EQ(data.column_end_idx, 2);
    EXPECT_TRUE(data.is_invalid);
  }
  {
    temp_file_manager manager("a\nb");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 2, 3);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.source_line, "b");
    EXPECT_EQ(data.line_idx, 1);
    EXPECT_EQ(data.column_start_idx, 0);
    EXPECT_EQ(data.column_end_idx, 1);
    EXPECT_FALSE(data.is_invalid);
  }
  {
    temp_file_manager manager("a\nb\n");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 2, 3);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.source_line, "b");
    EXPECT_EQ(data.line_idx, 1);
    EXPECT_EQ(data.column_start_idx, 0);
    EXPECT_EQ(data.column_end_idx, 1);
    EXPECT_FALSE(data.is_invalid);
  }
  {
    temp_file_manager manager("a\nb\n");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 4, 5);
    const diag_data& data = result.get_diag_data();
    EXPECT_FALSE(data.has_line());
    EXPECT_FALSE(data.has_column());
    EXPECT_FALSE(data.is_column_range());
    EXPECT_TRUE(data.has_file_name());
    EXPECT_TRUE(data.is_invalid);
  }
  {
    temp_file_manager manager("a\nstring\n");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 3, 6);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.source_line, "string");
    EXPECT_EQ(data.line_idx, 1);
    EXPECT_EQ(data.column_start_idx, 1);
    EXPECT_EQ(data.column_end_idx, 4);
    EXPECT_TRUE(data.is_column_range());
    EXPECT_FALSE(data.is_invalid);
  }
  {
    temp_file_manager manager("a\nstr\nstring");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type, 7, 10);
    const diag_data& data = result.get_diag_data();
    EXPECT_EQ(data.source_line, "string");
    EXPECT_EQ(data.line_idx, 2);
    EXPECT_EQ(data.column_start_idx, 1);
    EXPECT_EQ(data.column_end_idx, 4);
    EXPECT_TRUE(data.is_column_range());
    EXPECT_FALSE(data.is_invalid);
  }
  {
    temp_file_manager manager("abc\n");
    engine.set_file(&manager);
    diag_builder result = engine.create_diag(err_test_type);
    const diag_data& data = result.get_diag_data();
    EXPECT_FALSE(data.has_line());
    EXPECT_FALSE(data.has_column());
    EXPECT_TRUE(data.has_file_name());
    EXPECT_FALSE(data.is_column_range());
    EXPECT_FALSE(data.is_invalid);
  }
}

TEST(diag_builder_test, param) {
  diag_engine engine;
  std::unique_ptr<test_diag_consumer> consumer = std::make_unique<test_diag_consumer>();
  engine.set_consumer(consumer.get());
  {
    engine.create_diag(err_test_with_param_type) << "param_value" << diag_build_finish;
    EXPECT_EQ(consumer->get_data().origin_diag_message,
              "This is a test error message with param: %0.");
    EXPECT_EQ(consumer->get_data().level, diag_data::ERROR);
    EXPECT_FALSE(consumer->get_data().is_invalid);
    EXPECT_EQ(consumer->get_data()._result_diag_message,
              "This is a test error message with param: param_value.");
  }
  // test '%%'
  {
    test_diag_builder::create("%%", consumer.get()) << diag_build_finish;
    EXPECT_EQ(consumer->get_data().origin_diag_message, "%%");
    EXPECT_EQ(consumer->get_data()._result_diag_message, "%");
    test_diag_builder::create("%%", consumer.get()) << "abc" << diag_build_finish;
    EXPECT_EQ(consumer->get_data()._result_diag_message, "%");
  }
  // test basic args
  {
    test_diag_builder::create("%1 %0 ab %3", consumer.get())
      << 1 << "abc" << diag_build_finish;
    EXPECT_EQ(consumer->get_data()._result_diag_message, "abc 1 ab %3");
    test_diag_builder::create("%1 %0 ab %3", consumer.get())
      << 1 << "ab" << "ignore" << diag_build_finish;
    EXPECT_EQ(consumer->get_data()._result_diag_message, "ab 1 ab %3");
    test_diag_builder::create("%1 %0 ab %3", consumer.get())
      << 1 << "abc" << "ignore" << "bcd" << diag_build_finish;
    EXPECT_EQ(consumer->get_data()._result_diag_message, "abc 1 ab bcd");
    test_diag_builder::create("%1 %0 ab %3", consumer.get())
      << 1 << "abc" << "ignore" << 5 << "ignore" << diag_build_finish;
    EXPECT_EQ(consumer->get_data()._result_diag_message, "abc 1 ab 5");
  }
  // special cases
  {
    test_diag_builder::create("ab%", consumer.get()) << 1 << diag_build_finish;
    EXPECT_EQ(consumer->get_data()._result_diag_message, "ab%");
    test_diag_builder::create("ab%%0", consumer.get()) << 1 << diag_build_finish;
    EXPECT_EQ(consumer->get_data()._result_diag_message, "ab%0");
    test_diag_builder::create("ab", consumer.get()) << 1 << diag_build_finish;
    EXPECT_EQ(consumer->get_data()._result_diag_message, "ab");
  }
}

TEST(diag_builder_test, fix_hint) {
  diag_engine engine;
  std::unique_ptr<test_diag_consumer> consumer = std::make_unique<test_diag_consumer>();
  engine.set_consumer(consumer.get());
  {
    temp_file_manager manager("ab\nand a new line");
    engine.set_file(&manager);
    engine.create_diag(err_test_type)
      << engine.create_insertion_after_location(4, "insert")
      << diag_build_finish;
    {
      const diag_data& data = consumer->get_data();
      EXPECT_EQ(data.origin_diag_message,
                "This is a test error message.");
      EXPECT_TRUE(data.has_fix_hint());
      EXPECT_EQ(data.fix.replace_range,
                (std::pair<std::size_t, std::size_t>{2, 3}));
      EXPECT_EQ(data.fix.code_to_insert, "insert");
    }
    engine.create_diag(err_test_type)
      << engine.create_replacement(0, 5, "insert2")
      << diag_build_finish;
    {
      const diag_data& data = consumer->get_data();
      EXPECT_TRUE(data.has_fix_hint());
      EXPECT_EQ(data.fix.replace_range,
                (std::pair<std::size_t, std::size_t>{0, 5}));
      EXPECT_EQ(data.fix.code_to_insert, "insert2");
    }
    engine.create_diag(err_test_type)
        << engine.create_replacement(6, 10, "insert3")
        << diag_build_finish;
    {
      const diag_data& data = consumer->get_data();
      EXPECT_TRUE(data.has_fix_hint());
      EXPECT_EQ(data.fix.replace_range,
                (std::pair<std::size_t, std::size_t>{3, 7}));
      EXPECT_EQ(data.fix.code_to_insert, "insert3");
    }
    engine.create_diag(err_test_type)
        << engine.create_replacement(6, 4, "insert4")
        << diag_build_finish;
    {
      const diag_data& data = consumer->get_data();
      EXPECT_FALSE(data.has_fix_hint());
    }
    engine.create_diag(err_test_type) << diag_build_finish;
    {
      const diag_data& data = consumer->get_data();
      EXPECT_FALSE(data.has_fix_hint());
    }
  }
}

INTERPRETER_NAMESPACE_END
