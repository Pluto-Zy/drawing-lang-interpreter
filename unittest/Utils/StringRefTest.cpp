#include <Utils/StringRef.h>
#include <gtest/gtest.h>

INTERPRETER_NAMESPACE_BEGIN

// Check if string_ref can be constructed or assigned from temporary string.
static_assert(!std::is_constructible_v<string_ref, std::string>,
              "string_ref: cannot be constructed from prvalue std::string");
static_assert(!std::is_constructible_v<string_ref, std::string&&>,
              "string_ref: cannot be constructed from xvalue std::string");
static_assert(std::is_constructible_v<string_ref, std::string&>,
              "string_ref: can be constructed from lvalue std::string");
static_assert(std::is_constructible_v<string_ref, const char*>,
              "string_ref: can be constructed from prvalue raw string");
static_assert(std::is_constructible_v<string_ref, const char*&&>,
              "string_ref: can be constructed from xvalue raw string");
static_assert(std::is_constructible_v<string_ref, const char*&>,
              "string_ref: can be constructed from lvalue raw string");
static_assert(!std::is_assignable_v<string_ref&, std::string>,
              "string_ref: cannot be assigned from prvalue std::string");
static_assert(!std::is_assignable_v<string_ref&, std::string&&>,
              "string_ref: cannot be assigned from xvalue std::string");
static_assert(std::is_assignable_v<string_ref&, std::string&>,
              "string_ref: can be assigned from lvalue std::string");
static_assert(std::is_assignable_v<string_ref&, const char*>,
              "string_ref: can be assigned from prvalue raw string");
static_assert(std::is_assignable_v<string_ref&, const char*&&>,
              "string_ref: can be assigned from xvalue raw string");
static_assert(std::is_assignable_v<string_ref&, const char*&>,
              "string_ref: can be assigned from lvalue raw string");

namespace {
TEST(string_ref_test, construction) {
  EXPECT_EQ(string_ref(), "");
  EXPECT_EQ(string_ref("abc"), "abc");
  EXPECT_EQ(string_ref("abcde", 3), "abc");
  std::string str = "abc";
  EXPECT_EQ(string_ref(str), "abc");
}

TEST(string_ref_test, conversion) {
  EXPECT_EQ(std::string(string_ref("abc")), "abc");
}

TEST(string_ref_test, empty_str) {
  string_ref empty_str = {};
  EXPECT_TRUE(empty_str.empty());
  empty_str = {};
  EXPECT_TRUE(empty_str.empty());
  EXPECT_TRUE(string_ref().empty());
}

TEST(string_ref_test, iteration) {
  const char* src = "abcdef", *ptr = src;
  string_ref str = src;
  for (auto iter = str.begin(); iter != str.end(); ++iter, ++ptr) {
    EXPECT_EQ(*iter, *ptr);
  }
  for (string_ref::size_type i = 0; i < str.size(); ++i) {
    EXPECT_EQ(str[i], src[i]);
  }
}

TEST(string_ref_test, compare) {
  const char* str = "abcde";
  EXPECT_EQ(str, string_ref(str, 0).data());
  EXPECT_EQ(string_ref(str).size(), 5);

  EXPECT_LT(string_ref("aab").compare("aac"), 0);
  EXPECT_GT(string_ref("aab").compare("aaa"), 0);
  EXPECT_EQ(string_ref("aab").compare("aab"), 0);
  EXPECT_LT(string_ref("aab").compare("aaba"), 0);
  EXPECT_GT(string_ref("aab").compare("aa"), 0);

  EXPECT_LT(string_ref("AaB").compare_insensitive("aAc"), 0);
  EXPECT_LT(string_ref("AaB").compare_insensitive("aaBa"), 0);
  EXPECT_LT(string_ref("AaB").compare_insensitive("bb"), 0);
  EXPECT_GT(string_ref("AaB").compare_insensitive("AAA"), 0);
  EXPECT_GT(string_ref("aaBb").compare_insensitive("AaB"), 0);
  EXPECT_GT(string_ref("bb").compare_insensitive("AaB"), 0);
  EXPECT_GT(string_ref("AaB").compare_insensitive("aA"), 0);
  EXPECT_EQ(string_ref("AaB").compare_insensitive("aab"), 0);
}

TEST(string_ref_test, hash) {
  std::string s = "data";
  auto h = hash_value(static_cast<string_ref>(s));
  EXPECT_EQ(h, hash_value(string_ref("data")));
  EXPECT_NE(h, hash_value(string_ref("data ")));
  EXPECT_NE(h, hash_value(string_ref("dat")));
  EXPECT_NE(h, hash_value(string_ref("deta")));
  EXPECT_NE(h, hash_value("DatA"));
  EXPECT_EQ(h, hash_value_lower_case(string_ref("DatA")));
}

TEST(string_ref_test, edit_distance) {
  string_ref hello("hello");
  EXPECT_EQ(hello.edit_distance("hill"), 2U);
  EXPECT_EQ(hello.edit_distance("HiLl", /* ignore_cases = */ true), 2U);

  string_ref industry("industry");
  EXPECT_EQ(industry.edit_distance("interest"), 6U);
  EXPECT_EQ(industry.edit_distance("InTerESt", /* ignore_cases = */ true), 6U);

  string_ref soylent("soylent green is people");
  EXPECT_EQ(19U, soylent.edit_distance("people soiled our green"));
  EXPECT_EQ(26U, soylent.edit_distance("people soiled our green",
                                       /* ignore_cases = */ true,
                                       /* allow_replacements = */ false));
  EXPECT_EQ(9U, soylent.edit_distance("people soiled our green",
                                      /* ignore_cases = */ false,
                                      /* allow_replacements = */ true,
                                      /* max_distance = */ 8));
  EXPECT_EQ(53U, soylent.edit_distance("people soiled our green "
                                       "people soiled our green "
                                       "people soiled our green "));
}
} // namespace

INTERPRETER_NAMESPACE_END
