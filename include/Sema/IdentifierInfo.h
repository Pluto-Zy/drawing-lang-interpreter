/**
 * This file defines the data structures used to represent
 * variables and functions and the symbol table.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_IDENTIFIERINFO_H
#define DRAWING_LANG_INTERPRETER_IDENTIFIERINFO_H

#include <Utils/StringRef.h>
#include <AST/Type.h>
#include <Lex/TokenKinds.h>
#include <Diagnostic/DiagEngine.h>
#include <functional>
#include <memory>
#include <cassert>
#include <utility>
#include <unordered_map>
#include <optional>

INTERPRETER_NAMESPACE_BEGIN

class expr;

/**
 * This structure used to pass the information needed
 * when making diagnostic information.
 */
struct diag_info_pack {
  diag_engine& engine;
  /**
   * Saves the locations of the corresponding parameters.
   * For a function call expression with n params, it will contains 2n elements,
   * which represent the locations of every argument.
   * For an assignment statement, it will contains 2 elements representing the
   * expression used to assign.
   */
  std::vector<std::size_t> param_loc;
  /**
   * Represents the result of the assignment. If the member is @code{false}, the
   * interpreter will not assign the value to the variable.
   */
  bool success = true;
};

/**
 * This class represents an internal function in the language.
 * It saves the return type and the type of all parameters.
 */
class function_info {
protected:
  type _return_type;
  std::vector<type> _param_types;

  function_info(std::vector<type> param, type ret)
      : _return_type(std::move(ret)),
        _param_types(std::move(param)) { }
public:
  using param_iterator = std::vector<type>::const_iterator;

  function_info() = delete;
  function_info(const function_info&) = delete;
  function_info(function_info&&) = delete;
  function_info& operator=(const function_info&) = delete;
  function_info& operator=(function_info&&) = delete;
  virtual ~function_info() = default;

  [[nodiscard]] const type& get_ret_type() const { return _return_type; }
  [[nodiscard]] std::size_t get_param_count() const { return _param_types.size(); }
  [[nodiscard]] param_iterator param_begin() const { return _param_types.begin(); }
  [[nodiscard]] param_iterator param_end() const { return _param_types.end(); }
  [[nodiscard]] const type& get_param_type(std::size_t idx) const
    { return _param_types[idx]; }

  [[nodiscard]] virtual std::any call(diag_info_pack& pack, std::vector<std::any> args) const = 0;
};

class variable_info {
protected:
  type _var_type;

  explicit variable_info(type t) : _var_type(std::move(t)) { }
public:
  variable_info() = delete;
  variable_info(const variable_info&) = delete;
  variable_info(variable_info&&) = delete;
  variable_info& operator=(const variable_info&) = delete;
  variable_info& operator=(variable_info&&) = delete;
  virtual ~variable_info() = default;

  [[nodiscard]] const type& get_type() const { return _var_type; }
  [[nodiscard]] virtual std::any get_value() const = 0;
  [[nodiscard]] virtual std::any take_value() = 0;
  [[nodiscard]] virtual bool is_constant() const = 0;
  virtual void set_value(diag_info_pack& pack, std::any value) = 0;
};

template<class Ret, class... Args>
class function_info_impl : public function_info {
  static_assert(!(std::is_reference_v<Ret> || ... || std::is_reference_v<Args>),
      "Cannot use reference as return type and parameter types.");
private:
  std::function<Ret(Args...)> _callee;
public:
  template<class Callee, std::enable_if_t<std::is_invocable_r_v<Ret, Callee&&, Args...>, int> = 0>
  explicit function_info_impl(Callee&& callee)
    : function_info(get_types<Args...>(), get_type<Ret>()),
    _callee(std::forward<Callee>(callee)) { }
private:
  template<std::size_t... Idx>
  [[nodiscard]] std::any _call_impl(std::vector<std::any> args,
                                    std::index_sequence<Idx...>) const {
    if constexpr (std::is_same_v<Ret, void>) {
      _callee(unpack_value<Args>(args[Idx])...);
      return {};
    } else
      return pack_value(_callee(unpack_value<Args>(args[Idx])...));
  }
  [[nodiscard]] std::any call(diag_info_pack&, std::vector<std::any> args) const override {
    assert(args.size() == sizeof...(Args));
    return _call_impl(std::move(args), std::make_index_sequence<sizeof...(Args)>());
  }
};

template<class Ret, class... Args>
class function_info_impl<Ret, diag_info_pack&, Args...> : public function_info {
  static_assert(!(std::is_reference_v<Ret> || ... || std::is_reference_v<Args>),
                "Cannot use reference as return type and parameter types.");
  std::function<Ret(diag_info_pack&, Args...)> _callee;
public:
  template<class Callee,
      std::enable_if_t<std::is_invocable_r_v<Ret, Callee&&, diag_info_pack&, Args...>, int> = 0>
  explicit function_info_impl(Callee&& callee)
      : function_info(get_types<Args...>(), get_type<Ret>()),
        _callee(std::forward<Callee>(callee)) { }
private:
  template<std::size_t... Idx>
  [[nodiscard]] std::any _call_impl(diag_info_pack& pack,
                                    std::vector<std::any> args,
                                    std::index_sequence<Idx...>) const {
    if constexpr (std::is_same_v<Ret, void>) {
      _callee(pack, unpack_value<Args>(args[Idx])...);
      return {};
    } else
      return pack_value(_callee(pack, unpack_value<Args>(args[Idx])...));
  }
  [[nodiscard]] std::any call(diag_info_pack& pack, std::vector<std::any> args) const override {
    assert(args.size() == sizeof...(Args));
    return _call_impl(pack, std::move(args), std::make_index_sequence<sizeof...(Args)>());
  }
};

namespace {
template<class Ty>
struct _value_type {
  using type = std::remove_pointer_t<Ty>;
};

template<class Ty>
struct _value_type<std::unique_ptr<Ty>> {
  using type = Ty;
};

template<class VarTy>
class variable_storage : public variable_info {
  static_assert(!std::is_reference_v<VarTy>, "Cannot use reference as variable type.");
protected:
  VarTy _value;

  template<class... Args>
  explicit variable_storage(Args&&... args)
    : variable_info(DRAWING get_type<typename _value_type<VarTy>::type>()), _value(std::forward<Args>(args)...) { }
};
} // namespace

template<class VarTy>
class constant_info_impl : public variable_storage<VarTy> {
  using base_t = variable_storage<VarTy>;
public:
  explicit constant_info_impl(VarTy value) : base_t(std::move(value)) { }

  [[nodiscard]] std::any get_value() const override {
    return pack_value(this->_value);
  }
  [[nodiscard]] std::any take_value() override {
    return pack_value(std::move(this->_value));
  }

  [[nodiscard]] bool is_constant() const override { return true; }
  void set_value(diag_info_pack& pack, std::any) override {
    // cannot set value to constant
    assert(pack.param_loc.size() == 2);
    pack.engine.create_diag(err_assign_constant, pack.param_loc[0]) << diag_build_finish;
    pack.success = false;
  }
};

template<class VarTy>
class variable_info_impl : public variable_storage<VarTy*> {
  using base_t = variable_storage<VarTy*>;
  std::function<bool(diag_info_pack&, const VarTy&)> _value_filter;
public:
  template<class Callable,
      std::enable_if_t<std::is_constructible_v<decltype(_value_filter), Callable&&>, int> = 0>
  variable_info_impl(VarTy& value, Callable&& filter)
      : base_t(&value), _value_filter(std::forward<Callable>(filter)) { }
  [[nodiscard]] std::any get_value() const override {
    assert(this->_value != nullptr);
    return pack_value(*this->_value);
  }
  [[nodiscard]] std::any take_value() override {
    // we cannot take the value of a variable
    return get_value();
  }
  [[nodiscard]] bool is_constant() const override { return false; }
  void set_value(diag_info_pack& pack, std::any value) override {
    auto v = unpack_value<VarTy>(std::move(value));
    if (!_value_filter || _value_filter(pack, v))
      *this->_value = std::move(v);
    else
      pack.success = false;
  }
};

/**
 * runtime_variable_info_impl - stores the first packed level of the variable defined at runtime.
 * Since we cannot get the correct @code{type} object from the type of the internal variable,
 * we must provide the @code{type} object explicitly.
 */
template<class VarTy>
class runtime_variable_info_impl : public variable_info {
  std::unique_ptr<VarTy> _value;
public:
  template<class... Args>
  explicit runtime_variable_info_impl(type value_type, Args&&... args)
      : variable_info(std::move(value_type)), _value(std::make_unique<VarTy>(std::forward<Args>(args)...)) { }
  [[nodiscard]] std::any get_value() const override {
    assert(_value);
    return std::make_any<VarTy>(*_value);
  }
  [[nodiscard]] std::any take_value() override {
    // we cannot take the value of a variable
    return get_value();
  }
  [[nodiscard]] bool is_constant() const override { return false; }
  void set_value(diag_info_pack& pack, std::any value) override {
    //auto v = unpack_value<VarTy>(std::move(value));
    //*this->_value = std::move(v);
    *_value = std::any_cast<VarTy>(std::move(value));
  }
};

class symbol_table {
public:
  symbol_table();

  void add_variable(token_kind kind, string_ref spelling,
                    std::unique_ptr<variable_info> info);
  void add_function(token_kind kind, string_ref spelling,
                    std::unique_ptr<function_info> info);
  [[nodiscard]] variable_info*
  get_variable(token_kind kind, string_ref spelling) const;
  [[nodiscard]] std::vector<const function_info*>
  get_function(token_kind kind, string_ref spelling) const;
  [[nodiscard]] variable_info*
  get_variable(string_ref spelling) const;
  [[nodiscard]] std::vector<const function_info*>
  get_function(string_ref spelling) const;
  [[nodiscard]] bool has_variable(token_kind kind, string_ref spelling) const;
  [[nodiscard]] bool has_function(token_kind kind, string_ref spelling) const;
  [[nodiscard]] bool has_variable(string_ref spelling) const;
  [[nodiscard]] bool has_function(string_ref spelling) const;
  template<class OutIter, class Fn>
  OutIter get_var_if(OutIter out_beg, Fn f) const {
    for (const auto& [name, info] : _var_symbols) {
      if (f(name, info.get()))
        *out_beg++ = std::make_pair(name, info.get());
    }
    return out_beg;
  }
  template<class OutIter, class Fn>
  OutIter get_func_if(OutIter out_beg, Fn f) const {
    for (const auto& [name, overload] : _func_symbols) {
      for (const auto& func : overload) {
        if (f(name, func.get()))
          *out_beg++ = std::make_pair(name, func.get());
      }
    }
    return out_beg;
  }
private:
  std::unordered_map<string_ref, std::unique_ptr<variable_info>,
      decltype(hash_value)*> _var_symbols;
  std::unordered_map<string_ref, std::vector<std::unique_ptr<function_info>>,
      decltype(hash_value)*> _func_symbols;
};

template<class Ret, class... Args>
std::unique_ptr<function_info> make_info_from_func(Ret(* ptr)(Args...)) {
  return std::make_unique<function_info_impl<Ret, Args...>>(ptr);
}

template<class Ret, class Class, class... Args>
std::unique_ptr<function_info> make_info_from_mem_func(Class* obj, Ret(Class::* ptr)(Args...)) {
  return std::make_unique<function_info_impl<Ret, Args...>>([obj, ptr](Args... args) -> Ret {
    return (obj->*ptr)(args...);
  });
}
template<class Ret, class Class, class... Args>
std::unique_ptr<function_info> make_info_from_mem_func(const Class* obj, Ret(Class::* ptr)(Args...) const) {
  return std::make_unique<function_info_impl<Ret, Args...>>([obj, ptr](Args... args) -> Ret {
      return (obj->*ptr)(args...);
  });
}

template<class Ty>
std::unique_ptr<variable_info> make_info_from_constant(Ty val) {
  return std::make_unique<constant_info_impl<Ty>>(val);
}

template<class Ty, class Callable>
std::unique_ptr<variable_info> make_info_from_var(Ty& value, Callable&& filter) {
  return std::make_unique<variable_info_impl<Ty>>(value, std::forward<Callable>(filter));
}
template<class Ty>
std::unique_ptr<variable_info> make_info_from_var(Ty& value) {
  // no value filter
  return make_info_from_var(value, nullptr);
}

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_IDENTIFIERINFO_H
