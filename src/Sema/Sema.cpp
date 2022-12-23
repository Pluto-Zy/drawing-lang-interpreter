#include <Sema/Sema.h>
#include <Diagnostic/DiagData.h>
#include <stdexcept>
#include <algorithm>

INTERPRETER_NAMESPACE_BEGIN

namespace {

} // namespace

sema::sema(diag_engine& diag, symbol_table& table) :
  _diag_engine(diag), _symbol_table(table) {
}

bool sema::check_double_to_int(double value) {
  if (value <= std::numeric_limits<integer_type>::max() &&
      value >= std::numeric_limits<integer_type>::min()) {
    return value == static_cast<double>(static_cast<integer_type>(value));
  }
  return false;
}

variable_info* sema::add_new_variable(typed_value init_value, string_ref variable_name) {
  std::unique_ptr<variable_info> result;
  switch (init_value.get_type().get_kind()) {
#define VARIABLE_BASIC_TYPE(NAME, TYPE, SPELLING)                                     \
case type::NAME:                                                                      \
  result = std::make_unique<runtime_variable_info_impl<TYPE>>                         \
    (init_value.get_type(), unpack_value<TYPE>(std::move(init_value.take_value())));  \
  break;
#include <Interpret/TypeDef.h>
    case type::TUPLE:
      using tuple_t = std::vector<std::any>;
      result = std::make_unique<runtime_variable_info_impl<tuple_t>>
        (init_value.get_type(), std::any_cast<tuple_t>(std::move(init_value.take_value())));
      break;
    default:    // for void
      assert(false);    // unreachable
  }
  variable_info* result_ptr = result.get();
  _symbol_table.add_variable(token_kind::tk_identifier, variable_name, std::move(result));
  return result_ptr;
}

INTERPRETER_NAMESPACE_END
