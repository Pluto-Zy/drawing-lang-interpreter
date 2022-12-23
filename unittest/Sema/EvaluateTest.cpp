#include <MockTools.h>
#include <Sema/Sema.h>
#include <Interpret/InternalSupport/InternalImpl.h>
#include <Interpret/Interpreter.h>
#include <sstream>

INTERPRETER_NAMESPACE_BEGIN

namespace {
class EvaluateTest : public ::testing::Test {
protected:
  diag_engine engine;
  test_diag_consumer consumer;
  std::unique_ptr<test_file_manager> manager;
  std::unique_ptr<lexer> l;

  void SetUp() override {
    engine.set_consumer(&consumer);
  }

  template<std::size_t N>
  parser generate_parser(const char(& str)[N]) {
    consumer.clear();
    manager = std::make_unique<test_file_manager>(str);
    engine.set_file(manager.get());
    l = std::make_unique<lexer>(manager.get(), engine);
    return parser(*l);
  }

  template<std::size_t N>
  std::optional<typed_value> evaluate(const char(& str)[N], symbol_table& table) {
    sema action(engine, table);
    auto ast = generate_parser(str).parse_expr();
    if (action.bind_expr_variables(ast.get()))
      return action.evaluate(ast.get());
    return std::nullopt;
  }

  template<std::size_t N>
  std::optional<typed_value> evaluate(const char(& str)[N]) {
    symbol_table _temp_table;
    return evaluate(str, _temp_table);
  }

  template<std::size_t N>
  void run(const char(& str)[N]) {
    symbol_table _table;
    internal_impl impl;
    impl.export_all_symbols(_table);
    sema action(engine, _table);
    interpreter i(action, impl);
    auto group = generate_parser(str).parse_program();
    i.run_stmts(std::move(group));
    for (std::size_t i = 0; i < consumer.get_data_size(); ++i) {
      std::cout << consumer.get_data(i)._result_diag_message << std::endl;
    }
  }
};

TEST_F(EvaluateTest, number) {
  {
    char code[] = "1";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 1);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "1.0";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 1.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "2147483648";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 2147483648.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(1.5)";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 1.5);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
}

TEST_F(EvaluateTest, tuple) {
  {
    char code[] = "(1, 2, 3)";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::INTEGER));
    using real_t = std::vector<INTEGER_T>;
    real_t expected = {1, 2, 3};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(((1, 2, 3)))";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::INTEGER));
    using real_t = std::vector<INTEGER_T>;
    real_t expected = {1, 2, 3};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(1, 2.0, 3)";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<FLOAT_POINT_T>;
    real_t expected = {1, 2, 3};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = R"(("abc", "bcd", ""))";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::STRING));
    using real_t = std::vector<STRING_T>;
    real_t expected = {"abc", "bcd", ""};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "((1, 2), (3, 4))";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().get_sub_type().is(type::INTEGER));
    using real_t = std::vector<std::vector<INTEGER_T>>;
    real_t expected = {{1, 2},
                       {3, 4}};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "((1, 2.0), (3, 4))";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<std::vector<FLOAT_POINT_T>>;
    real_t expected = {{1, 2},
                       {3, 4}};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "((1, 2.0), (3, 4))";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<std::vector<FLOAT_POINT_T>>;
    real_t expected = {{1, 2},
                       {3, 4}};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "((1, 2.0), 3, 4)";
    auto result = evaluate(code);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
  }
  {
    char code[] = "(1, 2.0, (3))";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<FLOAT_POINT_T>;
    real_t expected = {1, 2, 3};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
}

TEST_F(EvaluateTest, String) {
  {
    char code[] = R"("abc")";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::STRING));
    EXPECT_EQ(unpack_value<STRING_T>(result->get_value()), "abc");
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = R"("ab\tc")";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::STRING));
    EXPECT_EQ(unpack_value<STRING_T>(result->get_value()), "ab\tc");
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
}

void vvfun() {}

TEST_F(EvaluateTest, variable) {
  INTEGER_T ival = 2;
  std::vector<std::vector<FLOAT_POINT_T>> fttval = {{1, 2},
                                                    {3, 4}};
  STRING_T sval = "abv";
  symbol_table table;
  table.add_variable(token_kind::tk_identifier, "ival", make_info_from_var(ival));
  table.add_variable(token_kind::tk_identifier, "fttval", make_info_from_var(fttval));
  table.add_variable(token_kind::tk_identifier, "sval", make_info_from_var(sval));
  table.add_function(token_kind::tk_identifier, "vvfun", make_info_from_func(&vvfun));
  {
    char code[] = "ival";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 2);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "sval";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::STRING));
    EXPECT_EQ(unpack_value<STRING_T>(result->get_value()), "abv");
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "fttval";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<std::vector<FLOAT_POINT_T>>;
    real_t expected = {{1, 2},
                       {3, 4}};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "attval";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<std::vector<FLOAT_POINT_T>>;
    real_t expected = {{1, 2},
                       {3, 4}};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  {
    char code[] = "vvfun";
    auto result = evaluate(code, table);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
}

INTEGER_T iglobaltest = 3;

INTEGER_T iiifun(INTEGER_T i1, INTEGER_T i2) { return i1 + i2; }

FLOAT_POINT_T ffffun(FLOAT_POINT_T f1, FLOAT_POINT_T f2) { return f1 + f2; }

INTEGER_T ivgetglobal() { return iglobaltest; }

VOID_T vvsetglobal(INTEGER_T val) { iglobaltest = val; }

INTEGER_T iiiifun(INTEGER_T i1, INTEGER_T i2, INTEGER_T i3) { return i1 + i2 + i3; }

FLOAT_POINT_T fffffun(FLOAT_POINT_T f1, FLOAT_POINT_T f2, FLOAT_POINT_T f3) { return f1 + f2 + f3; }


TEST_F(EvaluateTest, call) {
  symbol_table table;
  table.add_variable(token_kind::tk_identifier, "iglobaltest", make_info_from_var(iglobaltest));;
  table.add_function(token_kind::tk_identifier, "iiifun", make_info_from_func(&iiifun));
  table.add_function(token_kind::tk_identifier, "ffffun", make_info_from_func(&ffffun));
  table.add_function(token_kind::tk_identifier, "ivgetglobal", make_info_from_func(&ivgetglobal));
  table.add_function(token_kind::tk_identifier, "vvsetglobal", make_info_from_func(&vvsetglobal));
  table.add_function(token_kind::tk_identifier, "overload_add", make_info_from_func(&iiiifun));
  table.add_function(token_kind::tk_identifier, "overload_add", make_info_from_func(&fffffun));
  {
    char code[] = "iiifun(2, 3)";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 5);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "ivgetglobal()";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 3);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "vvsetglobal(10)";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::VOID));
    EXPECT_EQ(iglobaltest, 10);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "vvsetglobal(iiifun(iiifun(3, 5), 7))";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::VOID));
    EXPECT_EQ(iglobaltest, 15);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  // narrow
  {
    char code[] = "vvsetglobal(ffffun(3.5, 2.7))";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::VOID));
    EXPECT_EQ(iglobaltest, 6);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  // overload
  {
    char code[] = "overload_add(1, 2, 3)";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 6);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "overload_add(1.0, 2.5, 3.0)";
    auto result = evaluate(code, table);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 6.5);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "overload_add(1, 2.5, 3)";
    auto result = evaluate(code, table);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 3);
    for (std::size_t i = 0; i < consumer.get_data_size(); ++i) {
      std::cout << consumer.get_data(i)._result_diag_message << std::endl;
    }
  }
  {
    char code[] = "overload_add(1, 2.5)";
    auto result = evaluate(code, table);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 3);
    for (std::size_t i = 0; i < consumer.get_data_size(); ++i) {
      std::cout << consumer.get_data(i)._result_diag_message << std::endl;
    }
  }
  {
    char code[] = "overload_add(1, 2.5, (1, 2, 8))";
    auto result = evaluate(code, table);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 3);
    for (std::size_t i = 0; i < consumer.get_data_size(); ++i) {
      std::cout << consumer.get_data(i)._result_diag_message << std::endl;
    }
  }
  {
    char code[] = "iglobaltest()";
    auto result = evaluate(code, table);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    for (std::size_t i = 0; i < consumer.get_data_size(); ++i) {
      std::cout << consumer.get_data(i)._result_diag_message << std::endl;
    }
  }
  {
    char code[] = "iivfun(1, 2, 3)";
    auto result = evaluate(code, table);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    for (std::size_t i = 0; i < consumer.get_data_size(); ++i) {
      std::cout << consumer.get_data(i)._result_diag_message << std::endl;
    }
  }
}

TEST_F(EvaluateTest, add) {
  {
    char code[] = "2147483647 + 1";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 2147483648.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "1 + 5";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 6);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "1 + 5.0";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 6);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(1, 2, 3) + (2, 3, 4)";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::INTEGER));
    using real_t = std::vector<INTEGER_T>;
    real_t expected = {1, 2, 3, 2, 3, 4};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "((1, 2), (3.5, 4)) + ((1, 2.2), (5.0, 6.0, 3))";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<std::vector<FLOAT_POINT_T>>;
    real_t expected = {{1,   2},
                       {3.5, 4},
                       {1,   2.2},
                       {5,   6, 3}};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(1, 2, 3) + 3.0";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<FLOAT_POINT_T>;
    real_t expected = {4, 5, 6};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(1, 2, 2147483647) + 1";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<FLOAT_POINT_T>;
    EXPECT_DOUBLE_EQ(unpack_value<real_t>(result->get_value())[2], 2147483648.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = R"("ABC" + "abc""bcd" + "c")";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::STRING));
    EXPECT_EQ(unpack_value<STRING_T>(result->get_value()), "ABCabcbcdc");
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = R"("123" + 3.0)";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::STRING));
    EXPECT_EQ(unpack_value<STRING_T>(result->get_value()), "1233");
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = R"(4 + "abc" + (1 +2))";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::STRING));
    EXPECT_EQ(unpack_value<STRING_T>(result->get_value()), "4abc3");
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = R"((1, 2, 3.5) + "abc")";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::STRING));
    using real_t = std::vector<STRING_T>;
    real_t expected = {"1abc", "2abc", "3.5abc"};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = R"(("ab", "cd") + 5)";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::STRING));
    using real_t = std::vector<STRING_T>;
    real_t expected = {"ab5", "cd5"};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
}

TEST_F(EvaluateTest, sub) {
  {
    char code[] = "3 - 7";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), -4);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "3.0 - 7";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), -4.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(1, 2, 3) - 3.0";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<FLOAT_POINT_T>;
    real_t expected = {-2, -1, 0};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "3 - (1, 2, 3)";
    auto result = evaluate(code);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  {
    char code[] = R"(("A", "B") - 3)";
    auto result = evaluate(code);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  {
    char code[] = "-2147483647 - 1";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), -2147483647 - 1);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "-2147483648 - 1";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), -2147483649.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
}

TEST_F(EvaluateTest, multiply) {
  {
    char code[] = "4 * 8";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 32);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "4.0 * 8";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 32.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "4.0 * -8";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), -32.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(1, 3.0, 5) * 3";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<FLOAT_POINT_T>;
    real_t expected = {3, 9.0, 15};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "4.0 * ((1, 2), (3, 5), (4, 2))";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<std::vector<FLOAT_POINT_T>>;
    real_t expected = {{4.0,  8.0},
                       {12.0, 20.0},
                       {16.0, 8.0}};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(1, 2, 3) * (2, 3)";
    auto result = evaluate(code);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  {
    char code[] = R"(3 * "abc")";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::STRING));
    EXPECT_EQ(unpack_value<STRING_T>(result->get_value()), "abcabcabc");
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = R"(3.0 * "abc")";
    auto result = evaluate(code);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  {
    char code[] = R"((1, 3, 5) * "a")";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::STRING));
    using real_t = std::vector<STRING_T>;
    real_t expected = {"a", "aaa", "aaaaa"};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
}

TEST_F(EvaluateTest, division) {
  {
    char code[] = "8 / 4";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 2);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "4 / 8";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 0.5);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(1, 3.0, 2) / 3";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<FLOAT_POINT_T>;
    real_t expected = {1.0 / 3, 1.0, 2.0 / 3};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "4.0 / ((1, 2), (3, 5))";
    auto result = evaluate(code);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  {
    char code[] = "3 / 0";
    auto result = evaluate(code);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 2);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
    std::cout << consumer.get_data(1)._result_diag_message << std::endl;
  }
}

TEST_F(EvaluateTest, pow) {
  {
    char code[] = "3 ** 2";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 9);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "2.0 ** 4";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 16.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "(1, 3.0, 2) ** 3";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<FLOAT_POINT_T>;
    real_t expected = {1, 27.0, 8};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "4.0 ** ((1, 2), (3, 5))";
    auto result = evaluate(code);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  {
    char code[] = "3 ** 0";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 1);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "4 ** 0.5";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 2);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "10000 ** 3";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 1000000000000.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "3 ** 999";
    auto result = evaluate(code);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  {
    char code[] = "(-3) ** 0.5";
    auto result = evaluate(code);
    EXPECT_FALSE(result);
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
}

TEST_F(EvaluateTest, unary_plus) {
  {
    char code[] = "+8";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), 8);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "+4.0";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 4.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "+(1, 3.0, 2)";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::TUPLE));
    EXPECT_TRUE(result->get_type().get_sub_type().is(type::FLOAT_POINT));
    using real_t = std::vector<FLOAT_POINT_T>;
    real_t expected = {1, 3.0, 2};
    EXPECT_EQ(unpack_value<real_t>(result->get_value()), expected);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
}

TEST_F(EvaluateTest, unary_minus) {
  {
    char code[] = "-8";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::INTEGER));
    EXPECT_EQ(unpack_value<INTEGER_T>(result->get_value()), -8);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "-8.0";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), -8.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "- -2147483648";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 2147483648.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
  {
    char code[] = "- -2147483648";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 2147483648.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
}

TEST_F(EvaluateTest, Multi) {
  symbol_table table;
  table.add_variable(token_kind::tk_identifier, "iglobaltest", make_info_from_var(iglobaltest));;
  table.add_function(token_kind::tk_identifier, "iiifun", make_info_from_func(&iiifun));
  table.add_function(token_kind::tk_identifier, "ffffun", make_info_from_func(&ffffun));
  table.add_function(token_kind::tk_identifier, "ivgetglobal", make_info_from_func(&ivgetglobal));
  table.add_function(token_kind::tk_identifier, "vvsetglobal", make_info_from_func(&vvsetglobal));
  table.add_function(token_kind::tk_identifier, "overload_add", make_info_from_func(&iiiifun));
  table.add_function(token_kind::tk_identifier, "overload_add", make_info_from_func(&fffffun));
  {
    char code[] = "- -2147483648";
    auto result = evaluate(code);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->get_type().is(type::FLOAT_POINT));
    EXPECT_DOUBLE_EQ(unpack_value<FLOAT_POINT_T>(result->get_value()), 2147483648.0);
    EXPECT_EQ(consumer.get_data_size(), 0);
  }
}

} // namespace
INTERPRETER_NAMESPACE_END
