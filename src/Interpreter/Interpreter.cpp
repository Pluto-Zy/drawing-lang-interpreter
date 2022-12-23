#include <Interpret/Interpreter.h>

INTERPRETER_NAMESPACE_BEGIN


void interpreter::run_stmts(const std::vector<stmt_result_t>& stmts) {
  for (auto& stmt : stmts) {
    if (stmt)
      visit(stmt.get());
  }
}

bool interpreter::_assign_to_value(variable_expr* lhs, typed_value& rhs,
                                   std::size_t lhs_loc,
                                   std::size_t rhs_start_loc, std::size_t rhs_end_loc) {
  std::any _rhs_value;
  if (rhs.get_type() != lhs->get_bind_type()) {
    const type& to = lhs->get_bind_type();
    const type& from = rhs.get_type();
    if (!action.can_convert_to(from, to)) {
      diag(err_assign_incompatible_type, rhs_start_loc, rhs_end_loc)
          << to.get_spelling() << from.get_spelling() << diag_build_finish;
      return false;
    }
    std::string _origin_value = rhs.get_value_spelling();
    std::string _origin_type = rhs.get_type().get_spelling();
    bool narrow = false;
    typed_value convert_result = action.convert_to(std::move(rhs), to, narrow);
    _rhs_value = convert_result.take_value();
    if (narrow) {
      diag(warn_narrow_conversion, rhs_start_loc, rhs_end_loc)
          << _origin_type << convert_result.get_type().get_spelling()
          << _origin_value << convert_result.get_value_spelling() << diag_build_finish;
    }
  } else
    _rhs_value = rhs.take_value();
  // We don't need to check constant here.
  // because if we try to assign to a constant value, the default value
  // filter of it will report a diagnostic message.
  diag_info_pack pack = { action.get_diag_engine(),
                          { lhs->get_start_loc(), rhs_start_loc },
                          true };
  lhs->get_bind_info().set_value(pack, std::move(_rhs_value));
  return true;
}

void interpreter::visit_assignment_stmt(assignment_stmt* s) {
  assert(s);
  //bool bind_result = action.bind_expr_variables(s->get_assignment_lhs());
  //bind_result &= action.bind_expr_variables(s->get_assignment_rhs());
  if (!action.bind_expr_variables(s->get_assignment_rhs()))
    return;
  std::optional<typed_value> rhs = action.evaluate(s->get_assignment_rhs());
  if (!rhs)
    return;
  assert(s->get_assignment_lhs()->get_stmt_kind() == stmt::variable_expr_type);
  auto* lhs = static_cast<variable_expr*>(s->get_assignment_lhs());
  if (!action.try_bind_expr_variables(s->get_assignment_lhs())) {
    // we are defining a new variable now
    if (!_type_assignable(rhs->get_type())) {
      action.diag(err_deduced_variable_type, lhs->get_start_loc())
        << rhs->get_type().get_spelling() << diag_build_finish;
      return;
    }
    // create a new variable and bind to it.
    lhs->bind_to_variable(action.add_new_variable(std::move(*rhs), lhs->get_name()));
  } else {
    assert(lhs->has_bind_info());
    _assign_to_value(lhs, *rhs, lhs->get_start_loc(),
                     s->get_assignment_rhs()->get_start_loc(),
                     s->get_assignment_rhs()->get_end_loc());
  }
}

void interpreter::visit_expr_stmt(expr_stmt* s) {
  assert(s);
  expr* e = s->get_expr();
  if (!action.bind_expr_variables(e))
    return;
  (void)action.evaluate(e);
}

void interpreter::visit_for_stmt(for_stmt* s) {
  // steps:
  // 1. Calculate the value of the expression in each part of the for statement.
  // 2. If there is a from clause, assign it to the variable.
  // 3. Compare the variable with the value of 'to'.
  // 4. If current value is less than 'to', run the body.
  // 5. add the current value with the 'step' value, and goto 3.
  assert(s);
  // 1. Calculate the value of the expression in each part of the for statement.
  // bind variable to name
  bool bind_result = action.bind_expr_variables(s->get_for_expr());
  if (s->has_from())
    bind_result &= action.bind_expr_variables(s->get_from_expr());
  bind_result &= action.bind_expr_variables(s->get_to_expr());
  if (s->has_step())
    bind_result &= action.bind_expr_variables(s->get_step_expr());
  if (!bind_result)
    return;
  // evaluate
  assert(s->get_for_expr()->get_stmt_kind() == stmt::variable_expr_type);
  auto* for_variable = static_cast<variable_expr*>(s->get_for_expr());
  std::optional<typed_value> from_tv;
  if (s->has_from())
    from_tv = action.evaluate(s->get_from_expr());
  std::optional<typed_value> to_tv = action.evaluate(s->get_to_expr());
  std::optional<typed_value> step_tv;
  if (s->has_step())
    step_tv = action.evaluate(s->get_step_expr());
  else
    step_tv = typed_value(type(type::INTEGER), 1, true);
  if ((s->has_from() && !from_tv) || !to_tv || !step_tv)
    return;

  // 2. If there is a from clause, assign it to the variable.
  if (s->has_from()) {
    bool result = _assign_to_value(for_variable, *from_tv, for_variable->get_start_loc(),
                                   s->get_from_expr()->get_start_loc(),
                                   s->get_from_expr()->get_end_loc());
    if (!result)
      return;
  }

  // 3. Compare the variable with the value of 'to'.
  while (true) {
    int compare_result = action.compare(for_variable->get_bind_type(),
                                        for_variable->get_bind_value(),
                                        to_tv->get_type(), to_tv->get_value(),
                                        s->get_to_loc());
    // This is an invalid comparison, make a diag.
    if (compare_result == -2) {
      diag(err_invalid_compare_type, s->get_to_loc())
        << for_variable->get_bind_type().get_spelling()
        << to_tv->get_type().get_spelling() << diag_build_finish;
      return;
    }
    // stop loop
    if (compare_result >= 0)
      break;
    // 4. If current value is less than 'to', run the body.
    for (auto iter = s->body_begin(); iter != s->body_end(); ++iter) {
      visit(iter->get());
    }
    // 5. add the current value with the 'step' value, and goto 3.
    const type& lhs_type = for_variable->get_bind_type();
    const type& rhs_type = step_tv->get_type();
    std::size_t step_diag_report_loc = s->has_step() ? s->get_step_loc()
                                                     : s->get_for_expr()->get_start_loc();
    if (!action.can_add(lhs_type, rhs_type)) {
      diag(err_invalid_binary_operand, step_diag_report_loc)
        << lhs_type.get_spelling() << rhs_type.get_spelling() << diag_build_finish;
      return;
    }
    auto _add_result = action._add_unchecked(lhs_type, for_variable->get_bind_value(),
                                             rhs_type, step_tv->get_value(), step_diag_report_loc);
    if (!_add_result)
      return;
    // assign back
    bool _assign_back_result = _assign_to_value(for_variable, *_add_result,
                                                for_variable->get_start_loc(),
                                                step_diag_report_loc, step_diag_report_loc + 1);
    if (!_assign_back_result)
      return;
  }
}

bool interpreter::_type_assignable(const type& t) {
  switch (t.get_kind()) {
#define VARIABLE_BASIC_TYPE(KIND, TYPE, S) case type::KIND: return true;
#define BASIC_TYPE(KIND, TYPE, S) case type::KIND: return false;
#include <Interpret/TypeDef.h>
    case type::TUPLE:
      return true;
    default:
      assert(false);
      return false;
  }
}

INTERPRETER_NAMESPACE_END
