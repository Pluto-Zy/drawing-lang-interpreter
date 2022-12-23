#include <Sema/IdentifierInfo.h>
#include <unordered_set>
#include <algorithm>

INTERPRETER_NAMESPACE_BEGIN

symbol_table::symbol_table() :
  _var_symbols(10, &hash_value),
  _func_symbols(10, &hash_value) { }

void symbol_table::add_variable(token_kind kind, string_ref spelling,
                                std::unique_ptr<variable_info> info) {
  if (kind != token_kind::tk_identifier)
    spelling = get_spelling(kind);
  assert(_var_symbols.find(spelling) == _var_symbols.end());
  _var_symbols[spelling] = std::move(info);
}

void symbol_table::add_function(token_kind kind, string_ref spelling,
                                std::unique_ptr<function_info> info) {
  if (kind != token_kind::tk_identifier)
    spelling = get_spelling(kind);
#ifndef NDEBUG
  auto _checker = [](std::vector<std::unique_ptr<function_info>>& exist,
      std::unique_ptr<function_info>& insert) {
    for (auto& ptr : exist) {
      if (ptr->get_param_count() == insert->get_param_count() &&
          std::equal(ptr->param_begin(), ptr->param_end(), insert->param_begin()))
        return false;
    }
    return true;
  };
  assert(_checker(_func_symbols[spelling], info));
#endif
  _func_symbols[spelling].emplace_back(std::move(info));
}

variable_info*
symbol_table::get_variable(token_kind kind, string_ref spelling) const {
  if (kind != token_kind::tk_identifier)
    spelling = get_spelling(kind);
  return get_variable(spelling);
}

std::vector<const function_info*>
symbol_table::get_function(token_kind kind, string_ref spelling) const {
  if (kind != token_kind::tk_identifier)
    spelling = get_spelling(kind);
  return get_function(spelling);
}

variable_info* symbol_table::get_variable(string_ref spelling) const {
  if (auto iter = _var_symbols.find(spelling); iter != _var_symbols.end())
    return iter->second.get();
  return nullptr;
}

std::vector<const function_info*>
symbol_table::get_function(string_ref spelling) const {
  auto iter = _func_symbols.find(spelling);
  if (iter == _func_symbols.end())
    return {};
  const std::vector<std::unique_ptr<function_info>>& overload_func = iter->second;
  std::vector<const function_info*> result(overload_func.size());
  std::transform(overload_func.begin(), overload_func.end(), result.begin(),
                 std::mem_fn(&std::unique_ptr<function_info>::get));
  return result;
}

bool symbol_table::has_variable(token_kind kind, string_ref spelling) const {
  if (kind != token_kind::tk_identifier)
    spelling = get_spelling(kind);
  return has_variable(spelling);
}

bool symbol_table::has_function(token_kind kind, string_ref spelling) const {
  if (kind != token_kind::tk_identifier)
    spelling = get_spelling(kind);
  return has_function(spelling);
}

bool symbol_table::has_variable(string_ref spelling) const {
  return _var_symbols.find(spelling) != _var_symbols.end();
}

bool symbol_table::has_function(string_ref spelling) const {
  return _func_symbols.find(spelling) != _func_symbols.end();
}

INTERPRETER_NAMESPACE_END
