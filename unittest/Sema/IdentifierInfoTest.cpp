#include <Sema/IdentifierInfo.h>
#include <gtest/gtest.h>

INTERPRETER_NAMESPACE_BEGIN

namespace {

int _iget_for_test() { return 5; }
int _iadd_for_test(int n1, int n2) { return n1 + n2; }
double _fsub_for_test(double n1, double n2) { return n1 - n2; }
double _tuple_for_test(std::vector<double> n) {
  assert(n.size() == 3);
  return n[0] + n[1] + n[2];
}
double _multi_tuple_for_test(std::vector<std::vector<std::vector<double>>> n) {
  assert(n.size() == 3);
  double result = 0;
  for (const auto& v1 : n) {
    assert(v1.size() == 2);
    for (const auto& v2 : v1) {
      assert(v2.size() == 1);
      result += v2[0];
    }
  }
  return result;
}
int _idiv_for_test(diag_info_pack& pack, int n1, int n2) {
  if (n2 == 0) {
    pack.success = false;
    return 0;
  }
  return n1 / n2;
}
std::vector<std::vector<std::vector<double>>> _multi_ret_for_test(double val) {
  std::vector<double> v0 { val };
  std::vector<std::vector<double>> v1(2, v0);
  std::vector<std::vector<std::vector<double>>> v2(3, v1);
  return v2;
}

struct test_struct {
  int value;
  int get_value_for_test() { return value; }
  int get_value_for_test_const() const { return value; }
};

TEST(IdentifierInfo, function) {
  diag_engine engine;
  diag_info_pack pack { engine };
  {
    auto func = make_info_from_func(&_iget_for_test);
    EXPECT_TRUE(func);
    EXPECT_EQ(func->get_ret_type().get_kind(), type::INTEGER);
    EXPECT_EQ(func->get_param_count(), 0);
    std::any _call_result = func->call(pack, {});
    EXPECT_EQ(std::any_cast<int>(_call_result), 5);
    EXPECT_TRUE(pack.success);
  }
  {
    auto func = make_info_from_func(&_iadd_for_test);
    EXPECT_TRUE(func);
    EXPECT_EQ(func->get_ret_type().get_kind(), type::INTEGER);
    EXPECT_EQ(func->get_param_count(), 2);
    std::vector<type> param_type;
    param_type.emplace_back(type::INTEGER);
    param_type.emplace_back(type::INTEGER);
    EXPECT_TRUE(std::equal(param_type.begin(), param_type.end(), func->param_begin()));
    std::any _call_result = func->call(pack, {1, 2});
    EXPECT_EQ(std::any_cast<int>(_call_result), 3);
    EXPECT_TRUE(pack.success);
  }
  {
    auto func = make_info_from_func(&_fsub_for_test);
    EXPECT_TRUE(func);
    EXPECT_EQ(func->get_ret_type().get_kind(), type::FLOAT_POINT);
    EXPECT_EQ(func->get_param_count(), 2);
    std::vector<type> param_type;
    param_type.emplace_back(type::FLOAT_POINT);
    param_type.emplace_back(type::FLOAT_POINT);
    EXPECT_TRUE(std::equal(param_type.begin(), param_type.end(), func->param_begin()));
    std::any _call_result = func->call(pack, {1.5, 3.0});
    EXPECT_EQ(std::any_cast<double>(_call_result), -1.5);
    EXPECT_TRUE(pack.success);
  }
  {
    auto func = make_info_from_func(&_tuple_for_test);
    EXPECT_TRUE(func);
    EXPECT_EQ(func->get_ret_type().get_kind(), type::FLOAT_POINT);
    EXPECT_EQ(func->get_param_count(), 1);
    EXPECT_EQ(func->get_param_type(0).get_kind(), type::TUPLE);
    EXPECT_EQ(func->get_param_type(0).get_sub_type().get_kind(), type::FLOAT_POINT);
    std::any _call_result = func->call(pack, { static_cast<std::any>(std::vector<std::any>{1.5, 2.5, 3.5}) });
    EXPECT_EQ(std::any_cast<double>(_call_result), 7.5);
    EXPECT_TRUE(pack.success);
  }
  {
    auto func = make_info_from_func(&_multi_tuple_for_test);
    EXPECT_TRUE(func);
    EXPECT_EQ(func->get_ret_type().get_kind(), type::FLOAT_POINT);
    EXPECT_EQ(func->get_param_count(), 1);
    EXPECT_EQ(func->get_param_type(0).get_kind(), type::TUPLE);
    EXPECT_EQ(func->get_param_type(0).get_sub_type().get_kind(), type::TUPLE);
    EXPECT_EQ(func->get_param_type(0).get_sub_type()
      .get_sub_type().get_kind(), type::TUPLE);
    EXPECT_EQ(func->get_param_type(0).get_sub_type()
      .get_sub_type().get_sub_type().get_kind(), type::FLOAT_POINT);
    std::vector<std::any> v2 = { 1.0 };
    std::vector<std::any> v1(2, v2);
    std::vector<std::any> v0(3, v1);
    auto result = func->call(pack, { static_cast<std::any>(v0) });
    EXPECT_EQ(std::any_cast<double>(result), 6);
    EXPECT_TRUE(pack.success);
    std::vector<std::vector<std::vector<double>>> v = { {{1}, {1}}, {{1}, {1}}, {{1}, {2}} };
    auto result2 = func->call(pack, { pack_value(v) });
    EXPECT_EQ(std::any_cast<double>(result2), 7);
    EXPECT_TRUE(pack.success);
  }
  {
    auto func = make_info_from_func(&_multi_ret_for_test);
    EXPECT_TRUE(func);
    EXPECT_EQ(func->get_param_count(), 1);
    EXPECT_EQ(func->get_param_type(0).get_kind(), type::FLOAT_POINT);
    EXPECT_EQ(func->get_ret_type().get_kind(), type::TUPLE);
    EXPECT_EQ(func->get_ret_type().get_kind(), type::TUPLE);
    EXPECT_EQ(func->get_ret_type().get_sub_type().get_kind(), type::TUPLE);
    EXPECT_EQ(func->get_ret_type().get_sub_type()
                  .get_sub_type().get_kind(), type::TUPLE);
    EXPECT_EQ(func->get_ret_type().get_sub_type()
                  .get_sub_type().get_sub_type().get_kind(), type::FLOAT_POINT);
    auto result = func->call(pack, { 3.0 });
    EXPECT_EQ(std::any_cast<double>(
        std::any_cast<std::vector<std::any>>(
            std::any_cast<std::vector<std::any>>(
                std::any_cast<std::vector<std::any>>(result)[2])[1])[0]), 3.0);
    EXPECT_EQ(
        unpack_value<std::vector<std::vector<std::vector<double>>>>(result)[2][1][0], 3.0);
    EXPECT_TRUE(pack.success);
  }
  {
    auto func = make_info_from_func(&_idiv_for_test);
    EXPECT_TRUE(func);
    EXPECT_EQ(func->get_ret_type().get_kind(), type::INTEGER);
    EXPECT_EQ(func->get_param_count(), 2);
    std::vector<type> param_type;
    param_type.emplace_back(type::INTEGER);
    param_type.emplace_back(type::INTEGER);
    EXPECT_TRUE(std::equal(param_type.begin(), param_type.end(), func->param_begin()));
    auto result = func->call(pack, { 3, 2 });
    EXPECT_EQ(unpack_value<int>(result), 1);
    EXPECT_TRUE(pack.success);
    result = func->call(pack, { 3, 0 });
    EXPECT_FALSE(pack.success);
    pack.success = true;
  }
  {
    test_struct obj{ 5 };
    auto func = make_info_from_mem_func(&obj, &test_struct::get_value_for_test);
    EXPECT_TRUE(func);
    EXPECT_EQ(func->get_ret_type().get_kind(), type::INTEGER);
    EXPECT_EQ(func->get_param_count(), 0);
    auto result = func->call(pack, { });
    EXPECT_EQ(unpack_value<int>(result), 5);
    EXPECT_TRUE(pack.success);
    obj.value = 3;
    result = func->call(pack, { });
    EXPECT_EQ(unpack_value<int>(result), 3);
    EXPECT_TRUE(pack.success);
  }
  {
    test_struct obj{ 5 };
    auto func = make_info_from_mem_func(&obj, &test_struct::get_value_for_test_const);
    EXPECT_TRUE(func);
    EXPECT_EQ(func->get_ret_type().get_kind(), type::INTEGER);
    EXPECT_EQ(func->get_param_count(), 0);
    auto result = func->call(pack, { });
    EXPECT_EQ(unpack_value<int>(result), 5);
    EXPECT_TRUE(pack.success);
    obj.value = 3;
    result = func->call(pack, { });
    EXPECT_EQ(unpack_value<int>(result), 3);
    EXPECT_TRUE(pack.success);
  }
}

template<class Ty>
std::unique_ptr<constant_info_impl<Ty>> ttt(Ty val) {
  return std::make_unique<constant_info_impl<Ty>>(val);
}

bool _value_filter_for_test(diag_info_pack& pack, INTEGER_T val) {
  return false;
}

TEST(IdentifierInfo, variable) {
  diag_engine engine;
  diag_info_pack pack { engine };
  {
    int val = -3;
    auto var = make_info_from_var(val);
    EXPECT_TRUE(var);
    EXPECT_EQ(var->get_type().get_kind(), type::INTEGER);
    EXPECT_EQ(std::any_cast<int>(var->get_value()), -3);
    val = 5;
    EXPECT_EQ(std::any_cast<int>(var->get_value()), 5);
    var->set_value(pack, std::make_any<int>(10));
    EXPECT_EQ(std::any_cast<int>(var->get_value()), 10);
    EXPECT_EQ(val, 10);
  }
  {
    int val = -3;
    auto var = make_info_from_var(val, &_value_filter_for_test);
    EXPECT_TRUE(var);
    EXPECT_EQ(var->get_type().get_kind(), type::INTEGER);
    EXPECT_EQ(std::any_cast<int>(var->get_value()), -3);
    var->set_value(pack, std::make_any<int>(10));
    EXPECT_EQ(val, -3);
  }
  {
    std::vector<double> val = {1, 2};
    auto var = make_info_from_var(val);
    EXPECT_TRUE(var);
    EXPECT_EQ(var->get_type().get_kind(), type::TUPLE);
    EXPECT_EQ(var->get_type().get_sub_type().get_kind(), type::FLOAT_POINT);
    EXPECT_EQ(std::any_cast<double>(std::any_cast<std::vector<std::any>>(var->get_value())[0]), val[0]);
    EXPECT_EQ(unpack_value<std::vector<double>>(var->get_value()), val);
    val = {1, 2};
    EXPECT_EQ(unpack_value<std::vector<double>>(var->get_value()), val);
    var->set_value(pack, pack_value(decltype(val){2, 3, 4}));
    EXPECT_EQ(val, (std::vector<double>{2, 3, 4}));
  }
  {
    std::vector<std::vector<std::vector<int>>> val = { { { 1, 2 } } };
    auto var = make_info_from_var(val);
    EXPECT_TRUE(var);
    EXPECT_EQ(var->get_type().get_kind(), type::TUPLE);
    EXPECT_EQ(var->get_type().get_sub_type().get_kind(), type::TUPLE);
    EXPECT_EQ(var->get_type().get_sub_type().get_sub_type().get_kind(), type::TUPLE);
    EXPECT_EQ(var->get_type().get_sub_type().get_sub_type().get_sub_type().get_kind(), type::INTEGER);
    EXPECT_EQ(unpack_value<decltype(val)>(var->get_value()), val);
  }
  {
    auto val = make_info_from_constant(3.0);
    EXPECT_TRUE(val);
    EXPECT_EQ(val->get_type().get_kind(), type::FLOAT_POINT);
    EXPECT_EQ(std::any_cast<double>(val->get_value()), 3.0);
  }
  {
    using vec_t = std::vector<std::vector<int>>;
    auto val = make_info_from_constant(vec_t{ { 1, 2 } });
    EXPECT_TRUE(val);
    EXPECT_EQ(val->get_type().get_kind(), type::TUPLE);
    EXPECT_EQ(val->get_type().get_sub_type().get_kind(), type::TUPLE);
    EXPECT_EQ(val->get_type().get_sub_type().get_sub_type().get_kind(), type::INTEGER);
    EXPECT_EQ(unpack_value<vec_t>(val->get_value()), (vec_t{ { 1, 2 } }));
    auto result = val->take_value();
    EXPECT_EQ(unpack_value<vec_t>(val->get_value()), vec_t{});
  }
}
/*
TEST(IdentifierInfo, symbol_table) {
  symbol_table table;
  table.add_symbol(static_cast<unsigned char>(token_kind::tk_identifier),
                   "_iadd_for_test", make_info_from_func(&_iadd_for_test));
  table.add_symbol(static_cast<unsigned char>(token_kind::tk_identifier),
                   "_mix_for_test", make_info_from_func(&_mix_for_test));
  using pair = std::pair<double, double>;
  std::pair<double, double> val{1.2, 3.8};
  table.add_symbol(static_cast<unsigned char>(token_kind::tk_identifier),
                   "val", make_info_from_var(val));

  double t_value = 3.0;
  table.add_symbol(static_cast<unsigned char>(token_kind::kw_t),
                   "abc", make_info_from_var(t_value));
  {
    EXPECT_FALSE(
        table.get_symbol(static_cast<unsigned char>(token_kind::tk_identifier), "abc"));
    EXPECT_FALSE(
        table.get_symbol(static_cast<unsigned char>(token_kind::kw_origin), ""));
    EXPECT_TRUE(
        table.get_symbol(static_cast<unsigned char>(token_kind::kw_t), "t"));
  }
  {
    auto sym = table.get_symbol(static_cast<unsigned char>(token_kind::tk_identifier),
                                 "_mix_for_test");
    ASSERT_TRUE(sym);
    EXPECT_TRUE(sym->is_function());
    EXPECT_FALSE(sym->is_variable());
    EXPECT_EQ(static_cast<token_kind>(sym->get_kind()), token_kind::tk_identifier);
    EXPECT_EQ(sym->get_name(), "_mix_for_test");
    const auto& func = sym->get_function();
    EXPECT_EQ(func.get_ret_type(), type::TUPLE);
    EXPECT_EQ(func.get_param_count(), 3);
    std::vector<type> param_type = {type::INTEGER, type::FLOAT_POINT, type::TUPLE};
    EXPECT_TRUE(std::equal(param_type.begin(), param_type.end(), func.param_begin()));
    std::any _call_result = func.call({1, 2.0, std::make_pair(4.4, 2.1)});
    EXPECT_EQ(std::any_cast<pair>(_call_result), std::make_pair(9.5, -7.5));
  }
  {
    auto sym = table.get_symbol(static_cast<unsigned char>(token_kind::kw_t), "other str");
    ASSERT_TRUE(sym);
    EXPECT_TRUE(sym->is_variable());
    EXPECT_FALSE(sym->is_function());
    EXPECT_EQ(static_cast<token_kind>(sym->get_kind()), token_kind::kw_t);
    EXPECT_EQ(sym->get_name(), "abc");
    const auto& t = sym->get_variable();
    EXPECT_EQ(t.get_type(), type::FLOAT_POINT);
    EXPECT_EQ(std::any_cast<double>(t.get_value()), t_value);
    t_value = 5;
    EXPECT_EQ(std::any_cast<double>(t.get_value()), t_value);
  }
  {
    std::vector<string_ref> all_var_name = { "abc", "val" };
    std::vector<string_ref> all_func_name = { "_iadd_for_test", "_mix_for_test" };
    auto all_var_sym = table.get_all_variable(false);
    EXPECT_EQ(all_var_sym.size(), all_var_name.size());
    for (std::size_t i = 0; i < all_var_name.size(); ++i) {
      ASSERT_TRUE(all_var_sym[i]);
      EXPECT_EQ(all_var_sym[i]->get_name(), all_var_name[i]);
    }
    auto all_func_sym = table.get_all_function(false);
    EXPECT_EQ(all_func_sym.size(), all_func_name.size());
    for (std::size_t i = 0; i < all_func_name.size(); ++i) {
      ASSERT_TRUE(all_func_sym[i]);
      EXPECT_EQ(all_func_sym[i]->get_name(), all_func_name[i]);
    }
  }
}
 */
} // namespace

INTERPRETER_NAMESPACE_END
