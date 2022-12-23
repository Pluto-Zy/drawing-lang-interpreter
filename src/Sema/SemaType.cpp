#include <Sema/Sema.h>
#include <iterator>
#include <algorithm>
#include <sstream>

INTERPRETER_NAMESPACE_BEGIN

std::optional<type> sema::find_common_type(const type& lhs, const type& rhs) const {
  // Two common types of the same type are themselves.
  if (lhs == rhs)
    return lhs;
  // There is no common type between `VOID` and other types
  if (lhs.is(type::VOID) || rhs.is(type::VOID)) {
    return std::nullopt;
  }
  // check for tuple
  if (lhs.is(type::TUPLE) || rhs.is(type::TUPLE)) {
    const auto& tuple_type = lhs.is(type::TUPLE) ? lhs : rhs;
    const auto& other_type = lhs.is(type::TUPLE) ? rhs : lhs;
    if (other_type.is_not(type::TUPLE)) {
      // there is no common type between `TUPLE` and non-`TUPLE`
      return std::nullopt;
    }

    auto _common_sub_type = find_common_type(tuple_type.get_sub_type(),
                                             other_type.get_sub_type());
    if (!_common_sub_type)
      return std::nullopt;
    return type(type::TUPLE, std::make_unique<type>(std::move(*_common_sub_type)));
  }
  // for basic type Integer, Double and String.
  // although we can convert a number to string,
  // but they don't have common type.
  if (lhs.is(type::STRING) || rhs.is(type::STRING))
    return std::nullopt;
  return type(type::FLOAT_POINT);
}

typed_value sema::convert_to(typed_value from, const type& to, bool& narrow) const {
  std::vector<std::any> value;
  value.emplace_back(from.take_value());
  std::vector<std::any> result = convert_to(std::move(value), from.get_type(), to, narrow);
  assert(result.size() == 1);
  return typed_value(to, result[0], from.is_constant());
}

std::vector<std::any>
sema::convert_to(std::vector<std::any> values, const type& src, const type& dst, bool& narrow) const {
  if (src == dst)
    return values;
  // tuple
  if (src.is(type::TUPLE)) {
    assert(dst.is(type::TUPLE));
    // Okay, now the **elements** of `values` is `std::vector<std::any>`
    for (std::any& v : values) {
      std::vector<std::any> tuple_elems = std::any_cast<std::vector<std::any>>(std::move(v));
      // convert all the elements
      std::vector<std::any> cvt_result = convert_to(std::move(tuple_elems), src.get_sub_type(),
                                                dst.get_sub_type(), narrow);
      v = std::make_any<std::vector<std::any>>(std::move(cvt_result));
    }
    return values;
  }
  // for basic type
  if (dst.is(type::INTEGER)) {
    assert(src.is(type::FLOAT_POINT));
    narrow = true;
    for (std::any& v : values) {
      v = std::make_any<INTEGER_T>(static_cast<INTEGER_T>(std::any_cast<FLOAT_POINT_T>(v)));
    }
  } else {
    assert(src.is(type::INTEGER));
    assert(dst.is(type::FLOAT_POINT));
    for (std::any& v : values) {
      v = std::make_any<FLOAT_POINT_T>(static_cast<FLOAT_POINT_T>(std::any_cast<INTEGER_T>(v)));
    }
  }
  return values;
}

int sema::get_match_level(const type& arg, const type& param) const {
  if (param == arg)
    return 0;
  if (can_convert_to(arg, param))
    return 1;
  return -1;
}

bool sema::can_convert_to(const type& from, const type& to) const {
  if (from == to)
    return true;
  if (from.is(type::TUPLE) && to.is(type::TUPLE)) {
    return can_convert_to(from.get_sub_type(), to.get_sub_type());
  } else if (from.is_not(type::TUPLE) && to.is_not(type::TUPLE)) {
    // basic type
    if (from.is(type::VOID) || to.is(type::VOID))
      return false;
    // Integer -> String  Error
    // Double  -> String  Error
    // Integer -> Double  OK
    // Double  -> Integer OK
    // String  -> Integer Error
    // String  -> Double  Error
    return from.is_not(type::STRING) && to.is_not(type::STRING);
  }
  return false;
}

std::vector<const function_info*>
sema::_get_candidate_functions(string_ref func_name, std::size_t func_name_loc) const {
  std::vector<const function_info*> candidates = _symbol_table.get_function(func_name);
  if (!candidates.empty()) {
    return candidates;
  }
  // check whether it is a variable
  if (_symbol_table.has_variable(func_name)) {
    diag(err_use_var_as_func, func_name_loc) << diag_build_finish;
    return {};
  }
  // check whether it is a typo
  auto _distance_checker =
      [_min_distance = 5u, func_name] (string_ref name, const function_info* info) mutable  {
    unsigned distance = func_name.edit_distance(name);
    if (distance <= _min_distance && distance < std::min(name.size(), func_name.size())) {
      _min_distance = distance;
      return true;
    }
    return false;
  };
  std::vector<std::pair<string_ref, const function_info*>> _typo_names;
  _symbol_table.get_func_if(std::back_inserter(_typo_names), _distance_checker);
  if (_typo_names.size() > 1) {
    if (func_name.edit_distance(_typo_names.back().first) ==
        func_name.edit_distance(_typo_names[_typo_names.size() - 2].first)) {
      diag(err_use_unknown_identifier, func_name_loc) << diag_build_finish;
      return {};
    }
  }
  if (_typo_names.empty()) {
    diag(err_use_unknown_identifier, func_name_loc) << diag_build_finish;
    return {};
  }
  diag(err_use_unknown_identifier_with_hint, func_name_loc)
    << _typo_names.front().first << diag_build_finish;
  return {};
}

std::vector<const function_info*>
sema::_get_viable_functions(std::vector<const function_info*> candidates,
                            const std::vector<const type*>& param_types,
                            string_ref func_name,
                            std::size_t func_name_loc) const {
  assert(!candidates.empty());
  auto _check_param_match =
    [this, &param_types](const function_info* info) -> std::size_t {
      for (std::size_t i = 0; i < info->get_param_count(); ++i) {
        if (!can_convert_to(*param_types[i], info->get_param_type(i)))
          return i;
      }
      return info->get_param_count();
    };
  auto _get_ordinal_number = [](std::size_t i) -> std::string {
    std::string result = std::to_string(i);
    auto last = i % 10;
    if (last == 1)
      return result + "st";
    if (last == 2)
      return result + "nd";
    if (last == 3)
      return result + "rd";
    return result + "th";
  };
  std::vector<const function_info*> result;
  std::vector<diag_builder> _match_info;
  _match_info.reserve(candidates.size());
  for (auto info : candidates) {
    assert(info);
    if (info->get_param_count() != param_types.size()) {
      _match_info.emplace_back(
          diag(note_candidate_func_param_count_mismatch, func_name_loc)
            << info->get_param_count() << param_types.size()
      );
      continue;
    }
    auto mismatch_idx = _check_param_match(info);
    if (mismatch_idx < info->get_param_count()) {
      _match_info.emplace_back(
          diag(note_candidate_func_param_type_mismatch, func_name_loc)
            << param_types[mismatch_idx]->get_spelling()
            << info->get_param_type(mismatch_idx).get_spelling()
            << _get_ordinal_number(mismatch_idx + 1)
      );
      continue;
    }
    result.push_back(info);
  }
  if (result.empty()) {
    diag(err_no_match_func, func_name_loc) << func_name << diag_build_finish;
    for (auto& builder : _match_info) {
      std::move(builder) << diag_build_finish;
    }
  }
  return result;
}

const function_info*
sema::_find_best_viable_function(std::vector<const function_info*> viable_funcs,
                                 const std::vector<const type*>& param_types,
                                 string_ref func_name, std::size_t func_name_loc) const {
  assert(!viable_funcs.empty());
  // check whether lhs is better than rhs
  auto _better = [this, &param_types](const function_info* lhs, const function_info* rhs) -> bool {
    int _sum_of_level_1 = 0, _sum_of_level_2 = 0;
    for (std::size_t i = 0; i < lhs->get_param_count(); ++i) {
      int level1 = get_match_level(*param_types[i], lhs->get_param_type(i));
      int level2 = get_match_level(*param_types[i], rhs->get_param_type(i));
      if (level1 > level2)
        return false;
      _sum_of_level_1 += level1;
      _sum_of_level_2 += level2;
    }
    return _sum_of_level_1 < _sum_of_level_2;
  };
  struct function_info_wrapper {
    const function_info* info;
    bool best = false;
  };
  std::vector<function_info_wrapper> _viable_wrapper;
  _viable_wrapper.reserve(viable_funcs.size());
  std::transform(viable_funcs.begin(), viable_funcs.end(), std::back_inserter(_viable_wrapper),
                 [](const function_info* info) -> function_info_wrapper { return { info, false }; });
  // Find the best viable function.
  auto best = _viable_wrapper.begin();
  for (auto iter = ++_viable_wrapper.begin(); iter != _viable_wrapper.end(); ++iter) {
    if (_better(iter->info, best->info))
      best = iter;
  }
  best->best = true;
  std::vector<function_info_wrapper*> pending_best = { &*best };
  bool ambiguous = false;
  while (!pending_best.empty()) {
    auto* cur = pending_best.back();
    pending_best.pop_back();
    for (auto& wrapper : _viable_wrapper) {
      if (!wrapper.best && !_better(cur->info, wrapper.info)) {
        pending_best.push_back(&wrapper);
        wrapper.best = true;
        ambiguous = true;
      }
    }
  }
  if (ambiguous) {
    // report error
    std::string argument_type_list = "(";
    std::string func_name_str = static_cast<std::string>(func_name);
    if (param_types.empty())
      argument_type_list += ')';
    else {
      argument_type_list += param_types.front()->get_spelling();
      for (std::size_t i = 1; i < param_types.size(); ++i) {
        argument_type_list += ", " + param_types[i]->get_spelling();
      }
      argument_type_list += ')';
    }
    diag(err_ambiguous_call, func_name_loc)
      << func_name_str + argument_type_list
      << diag_build_finish;
    // show candidates
    for (auto& wrapper : _viable_wrapper) {
      if (!wrapper.best)
        continue;
      // return type
      const function_info* info = wrapper.info;
      std::string func_spelling = info->get_ret_type().get_spelling();
      // name
      func_spelling += ' ' + func_name_str;
      // param list
      func_spelling += '(';
      if (info->get_param_count() == 0)
        func_spelling += ')';
      else {
        func_spelling += info->get_param_type(0).get_spelling();
        for (std::size_t i = 1; i < info->get_param_count(); ++i) {
          func_spelling += ", " + info->get_param_type(i).get_spelling();
        }
        func_spelling += ')';
      }
      diag(note_candidate) << func_spelling << diag_build_finish;
    }
    return nullptr;
  }
  return best->info;
}

const function_info*
sema::overload_resolution(string_ref func_name, std::size_t func_name_loc,
                          const std::vector<const type*>& param_types) const {
  std::vector<const function_info*> candidate = _get_candidate_functions(func_name, func_name_loc);
  if (candidate.empty())
    return nullptr;
  std::vector<const function_info*> viable = _get_viable_functions(std::move(candidate), param_types,
                                                                   func_name, func_name_loc);
  if (viable.empty())
    return nullptr;
  return _find_best_viable_function(std::move(viable), param_types, func_name, func_name_loc);
}

const function_info*
sema::overload_resolution(string_ref func_name, std::size_t func_name_loc,
                          const std::vector<typed_value>& param) const {
  std::vector<const type*> param_types;
  param_types.reserve(param.size());
  std::transform(param.begin(), param.end(), std::back_inserter(param_types),
                 [](const typed_value& tv) { return &tv.get_type(); });
  return overload_resolution(func_name, func_name_loc, param_types);
}

bool sema::can_add(const type& lhs, const type& rhs) const {
  // cannot add anything with `VOID`
  if (lhs.is(type::VOID) || rhs.is(type::VOID))
    return false;
  // basic type
  if (lhs.is_not(type::TUPLE) && rhs.is_not(type::TUPLE))
    return true;
  const type& tuple_type = lhs.is(type::TUPLE) ? lhs : rhs;
  const type& other_type = lhs.is(type::TUPLE) ? rhs : lhs;
  if (other_type.is(type::TUPLE)) {
    // add tuple with tuple
    // Only when the element types are the same, two tuples can
    // be added (this means splicing the two tuples)
    return tuple_type.get_sub_type() == other_type.get_sub_type();
  } else {
    // add tuple with a basic type
    // Since adding a tuple to a basic object means adding the object
    // to each element of the tuple, it is necessary to check
    // whether the element type of the tuple and the basic type
    // can be added.
    return can_add(tuple_type.get_sub_type(), other_type);
  }
}

std::optional<typed_value>
sema::tidy_tuple(const std::vector<typed_value>& tuple_elems,
                 std::vector<std::size_t> elem_loc) const {
  assert(tuple_elems.size() > 1);
  bool need_diag = elem_loc.size() == tuple_elems.size();
  // find the common type of the elements
  type _common_type = tuple_elems.front().get_type();
  for (std::size_t i = 1; i < tuple_elems.size(); ++i) {
    auto _c = find_common_type(_common_type, tuple_elems[i].get_type());
    if (!_c) {
      if (need_diag) {
        diag(err_conflict_tuple_elem_type, elem_loc[i])
          << _common_type.get_spelling() << tuple_elems[i].get_type().get_spelling()
          << diag_build_finish;
      }
      return std::nullopt;
    }
    _common_type = std::move(*_c);
  }
  bool narrow = false;
  bool constant = true;
  std::vector<std::any> _pack_result;
  _pack_result.reserve(tuple_elems.size());
  for (auto& v : tuple_elems) {
    constant &= v.is_constant();
    _pack_result.emplace_back(convert_to(v, _common_type, narrow).get_value());
    // there is no need to process the narrowing conversion here
    // because it will never happen.
    assert(!narrow);
  }
  return typed_value(type(type::TUPLE, std::make_unique<type>(std::move(_common_type))),
                     std::make_any<std::vector<std::any>>(std::move(_pack_result)),
                     constant);
}

bool sema::can_sub(const type& lhs, const type& rhs) const {
  // cannot subtract anything with `VOID`
  if (lhs.is(type::VOID) || rhs.is(type::VOID))
    return false;
  // basic type
  if (lhs.is_not(type::TUPLE) && rhs.is_not(type::TUPLE))
    return lhs.is_not(type::STRING) && rhs.is_not(type::STRING);  // cannot subtract anything with STRING
  // TUPLE - TUPLE   -> error
  // BASIC - TUPLE   -> error
  // TUPLE - BASIC   -> maybe OK
  if (rhs.is(type::TUPLE))
    return false;
  // now lhs is a tuple
  return can_sub(lhs.get_sub_type(), rhs);
}

bool sema::can_mul(const type& lhs, const type& rhs) const {
  // cannot multiply anything with 'VOID'
  if (lhs.is(type::VOID) || rhs.is(type::VOID))
    return false;
  // basic type
  // NUM * NUM -> OK
  // STR * INTEGER -> OK
  // INTEGER * STR -> OK
  // STR * FLOAT -> ERROR
  // FLOAT * STR -> ERROR
  // STR * STR -> ERROR
  if (lhs.is_not(type::TUPLE) && rhs.is_not(type::TUPLE)) {
    if (lhs.is(type::STRING))
      return rhs.is(type::INTEGER);
    if (rhs.is(type::STRING))
      return lhs.is(type::INTEGER);
    return true;
  }
  // with tuple
  const type& tuple_type = lhs.is(type::TUPLE) ? lhs : rhs;
  const type& other_type = lhs.is(type::TUPLE) ? rhs : lhs;
  // TUPLE * TUPLE  -> error
  if (other_type.is(type::TUPLE))
    return false;
  else
    return can_mul(tuple_type.get_sub_type(), other_type);
}

bool sema::can_div(const type& lhs, const type& rhs) const {
  // cannot div anything with `VOID`
  if (lhs.is(type::VOID) || rhs.is(type::VOID))
    return false;
  // basic type
  if (lhs.is_not(type::TUPLE) && rhs.is_not(type::TUPLE))
    return lhs.is_not(type::STRING) && rhs.is_not(type::STRING);  // only permit number division
  // TUPLE / TUPLE  -> error
  // BASIC / TUPLE  -> error
  // TUPLE / BASIC  -> maybe OK
  if (rhs.is(type::TUPLE))
    return false;
  return can_div(lhs.get_sub_type(), rhs);
}

bool sema::can_pow(const type& lhs, const type& rhs) const {
  // cannot pow anything with `VOID`
  if (lhs.is(type::VOID) || rhs.is(type::VOID))
    return false;
  // basic type
  if (lhs.is_not(type::TUPLE) && rhs.is_not(type::TUPLE))
    return lhs.is_not(type::STRING) && rhs.is_not(type::STRING);  // only permit number pow
  // TUPLE ** TUPLE  -> error
  // BASIC ** TUPLE  -> error
  // TUPLE ** BASIC  -> maybe OK
  if (rhs.is(type::TUPLE))
    return false;
  return can_pow(lhs.get_sub_type(), rhs);
}

bool sema::can_unary_plus(const type& op) const {
  // + VOID     -> error
  // + INTEGER  -> OK
  // + DOUBLE   -> OK
  // + TUPLE    -> maybe OK
  // + STRING   -> error
  if (op.is_not(type::TUPLE))
    return op.is_not(type::VOID) && op.is_not(type::STRING);
  return can_unary_plus(op.get_sub_type());
}

bool sema::can_unary_minus(const type& op) const {
  // - VOID     -> error
  // - INTEGER  -> OK
  // - DOUBLE   -> OK
  // - TUPLE    -> maybe OK
  // - STRING   -> error
  if (op.is_not(type::TUPLE))
    return op.is_not(type::VOID) && op.is_not(type::STRING);
  return can_unary_minus(op.get_sub_type());
}

INTERPRETER_NAMESPACE_END
