#include <gtest/gtest.h>
#include <Lex/Lexer.h>
#include <Diagnostic/DiagConsumer.h>
#include <Diagnostic/DiagData.h>

INTERPRETER_NAMESPACE_BEGIN

template<class str_type>
str_type generate_temp_file_name();

template<>
std::string generate_temp_file_name() {
  return "temp_lex_file";
}

template<>
std::wstring generate_temp_file_name() {
  return L"temp_lex_file";;
}

namespace {
class test_file_manager : public file_manager {
public:
  template<std::size_t N>
  test_file_manager(const char(&str)[N]) :
      file_manager(copy(str), N - 1,
                   generate_temp_file_name<
                       std::remove_cv_t<
                           std::remove_reference_t<
                               decltype(std::declval<file_manager>().get_file_name())>>>()) { }

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

class LexerTest : public ::testing::Test {
protected:
  diag_engine engine;
  test_diag_consumer consumer;
  std::unique_ptr<test_file_manager> manager;

  void SetUp() override {
    engine.set_consumer(&consumer);
  }

  template<std::size_t N>
  lexer generate_lexer(const char(&str)[N]) {
    manager = std::make_unique<test_file_manager>(str);
    engine.set_file(manager.get());
    return {manager.get(), engine};
  }

  std::vector<token> lex_all(lexer& l) const {
    std::vector<token> result;
    token t;
    for (l.lex_and_consume(t); t.is_not(token_kind::tk_eof); l.lex_and_consume(t))
      result.push_back(t);
    return result;
  }

  void check_same(std::vector<token_kind> expect, std::vector<token> result) const {
    EXPECT_EQ(expect.size(), result.size());
    for (std::size_t i = 0; i < expect.size(); ++i) {
      EXPECT_EQ(result[i].get_kind(), expect[i]) << "Index: " << i
        << " Value: " << (std::string)result[i].get_data();
    }
  }
};

TEST(TokenTest, token) {
  token t;
  t.set_kind(token_kind::op_minus);
  EXPECT_TRUE(t.is_operator());
  EXPECT_FALSE(t.is_keyword());
  t.set_kind(token_kind::kw_scale);
  EXPECT_TRUE(t.is_keyword());
  EXPECT_FALSE(t.is_operator());
  t.set_kind(token_kind::tk_identifier);
  EXPECT_FALSE(t.is_keyword());
  EXPECT_FALSE(t.is_operator());
}

TEST_F(LexerTest, lexIdentifier) {
  {
    lexer l = generate_lexer("abc abc123 _a_b\na_ _1 _A _ __ a1_2b_c abc+");
    std::vector<token_kind> expect(10, token_kind::tk_identifier);
    expect.push_back(token_kind::op_plus);
    check_same(expect, lex_all(l));
  }
  {
    lexer l = generate_lexer("origin OriGin scale sCAlE rot ROT is iS\v"
                             "to TO step sTeP draw DRaW for fOR from FRom t T\r"
                             "_to s_cale fro A");
    std::vector<token_kind> expect {
#define keyword(spelling) token_kind::kw_##spelling, token_kind::kw_##spelling,
#include <Lex/KeywordDef.h>
#undef keyword
      token_kind::tk_identifier, token_kind::tk_identifier,
      token_kind::tk_identifier, token_kind::tk_identifier
    };
    check_same(expect, lex_all(l));
  }
}

TEST_F(LexerTest, lexNum) {
  {
    lexer l = generate_lexer("0 345 0.0 12. 0.1 0.05 1.0 013 01.02 .3 1.1.1");
    std::vector<token_kind> expect(9, token_kind::tk_constant);
    expect.push_back(token_kind::tk_unknown);
    expect.insert(expect.end(), 2, token_kind::tk_constant);
    expect.push_back(token_kind::tk_unknown);
    expect.push_back(token_kind::tk_constant);
    check_same(expect, lex_all(l));
  }
}

TEST_F(LexerTest, lexOperator) {
  {
    lexer l = generate_lexer(";)(,-+/**** * **");
    std::vector<token_kind> expect = {
        token_kind::op_semi, token_kind::op_r_paren, token_kind::op_l_paren,
        token_kind::op_comma, token_kind::op_minus, token_kind::op_plus,
        token_kind::op_slash, token_kind::op_star_star, token_kind::op_star_star,
        token_kind::op_star, token_kind::op_star_star
    };
    check_same(expect, lex_all(l));
  }
}

TEST_F(LexerTest, lexOther) {
  // unknown characters
  {
    lexer l = generate_lexer(".\\><][?$%^#@");
    std::vector<token_kind> expect(12, token_kind::tk_unknown);
    check_same(expect, lex_all(l));
  }
  // comment
  {
    lexer l = generate_lexer("123---a+3tq\n-3-/a//com//12\r\n/-a-/--last");
    std::vector<token_kind> expect = {
        token_kind::tk_constant,
        token_kind::op_minus, token_kind::tk_constant, token_kind::op_minus,
        token_kind::op_slash, token_kind::tk_identifier,
        token_kind::op_slash, token_kind::op_minus, token_kind::tk_identifier,
        token_kind::op_minus, token_kind::op_slash
    };
    check_same(expect, lex_all(l));
  }
}

TEST_F(LexerTest, diag) {
  {
    lexer l = generate_lexer("abc\0 123");
    lex_all(l);
    const diag_data& data = consumer.get_data();
    EXPECT_EQ(data.level, diag_data::WARNING);
    EXPECT_EQ(data.column_start_idx, 3);
    EXPECT_EQ(data.column_end_idx, 4);
    EXPECT_EQ(data._result_diag_message, "null character ignored");
  }
  {
    lexer l = generate_lexer("123..2");
    auto result = lex_all(l);
    const diag_data& data = consumer.get_data();
    EXPECT_EQ(data.level, diag_data::ERROR);
    EXPECT_EQ(data.column_start_idx, 4);
    EXPECT_EQ(data.column_end_idx, 5);
    EXPECT_EQ(data._result_diag_message, "invalid character '.'");
    EXPECT_EQ(data.source_line, "123..2");
    check_same({token_kind::tk_constant, token_kind::tk_unknown, token_kind::tk_constant},
               result);
  }
}

TEST_F(LexerTest, cache) {
  // test `lookahead`
  {
    lexer l = generate_lexer("id 123num;--comment\n+3.");
    EXPECT_TRUE(l.look_ahead(1).is(token_kind::tk_identifier));
    EXPECT_TRUE(l.look_ahead(4).is(token_kind::op_semi));
    EXPECT_EQ(l.next_token(), l.look_ahead(1));
    l.consume();
    EXPECT_TRUE(l.next_token().is(token_kind::tk_constant));
    EXPECT_EQ(l.next_token(), l.next_token());
    EXPECT_TRUE(l.look_ahead(4).is(token_kind::op_plus));
    EXPECT_TRUE(l.look_ahead(5).is(token_kind::tk_constant));
    EXPECT_TRUE(l.look_ahead(6).is(token_kind::tk_eof));
  }
  // test `lex_until`
  {
    lexer l = generate_lexer("abc 123.2+-123+- --1");
    l.lex_until(token_kind::tk_constant);
    EXPECT_TRUE(l.next_token().is(token_kind::tk_constant));
    EXPECT_EQ(l.next_token().get_data(), "123.2");
    l.lex_until_and_consume(token_kind::op_minus);
    EXPECT_TRUE(l.next_token().is(token_kind::tk_constant));
    EXPECT_EQ(l.next_token().get_data(), "123");
    l.lex_until_and_consume(token_kind::op_minus);
    EXPECT_TRUE(l.next_token().is(token_kind::tk_eof));

    lexer l2 = generate_lexer("line 1+3//comment\t\nand a new line");
    l2.look_ahead(5);   // generate cache
    EXPECT_TRUE(l2.next_token().is(token_kind::tk_identifier));
    EXPECT_TRUE(l2.look_ahead(7).is(token_kind::tk_identifier));
    l2.consume();
    EXPECT_TRUE(l2.next_token().is(token_kind::tk_constant));
    l2.lex_until_eol();
    EXPECT_TRUE(l2.next_token().is(token_kind::tk_identifier));
    EXPECT_EQ(l2.next_token().get_data(), "and");
    l2.lex_until_eol();
    EXPECT_TRUE(l2.next_token().is(token_kind::tk_eof));
  }
}

} // namespace
INTERPRETER_NAMESPACE_END
