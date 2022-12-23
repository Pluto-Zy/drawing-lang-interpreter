#include <Parse/Parser.h>
#include <Diagnostic/DiagData.h>

INTERPRETER_NAMESPACE_BEGIN

// Handle errors here by skipping up to the next ';' or '}',
// and eat the ';' if that's what stopped us.
#define STMT_SKIP                                       \
skip_until(token_kind::op_semi, token_kind::op_r_brace, \
/* stop_before_match = */true);                         \
if (tok.is(token_kind::op_semi))                        \
  consume_token();


parser::parser(lexer& _l) :
  _lexer(_l),
  prev_tok_loc(0),
  _diag_engine(_l.get_diag_engine()),
  _paren_count(0), _brace_count(0) {
  _lexer.lex_and_consume(tok);
}

std::size_t parser::consume_token() {
  if (_is_token_paren())
    return _consume_paren();
  if (_is_token_brace())
    return _consume_brace();
  prev_tok_loc = tok.get_end_location();
  _lexer.lex_and_consume(tok);
  return prev_tok_loc;
}

std::size_t parser::_consume_paren() {
  assert(_is_token_paren());
  if (tok.is(token_kind::op_l_paren))
    ++_paren_count;
  else if (_paren_count)
    --_paren_count;
  prev_tok_loc = tok.get_end_location();
  _lexer.lex_and_consume(tok);
  return prev_tok_loc;
}

std::size_t parser::_consume_brace() {
  assert(_is_token_brace());
  if (tok.is(token_kind::op_l_brace))
    ++_brace_count;
  else if (_brace_count)
    --_brace_count;
  prev_tok_loc = tok.get_end_location();
  _lexer.lex_and_consume(tok);
  return prev_tok_loc;
}

token parser::next_token() {
  return _lexer.next_token();
}

bool parser::expect_and_consume(token_kind expected_tok, diag_id diag,
                                string_ref expected, string_ref after,
                                bool check_typo) {
  if (tok.is(expected_tok)) {
    consume_token();
    return true;
  }
  if (check_typo && maybe_typo(tok, expected_tok)) {
    this->diag(diag, tok.get_start_location(), tok.get_start_location())
      << expected << after
      << _diag_engine.create_replacement(tok.get_start_location(),
                                         tok.get_end_location(),
                                         get_spelling(expected_tok)) << diag_build_finish;
    consume_token();
    return true;
  }
  this->diag(diag, prev_tok_loc) << expected << after << diag_build_finish;
  return false;
}

bool parser::expect_right_paren_and_consume(std::size_t l_paren_loc) {
  if (tok.is(token_kind::op_r_paren)) {
    consume_token();
    return true;
  }
  // There is currently no typo correction strategy for right parentheses.
  // error: expected ')'
  diag(err_expect, tok.get_start_location()) << ')' << diag_build_finish;
  // note: to match this '('
  diag(note_match_l_paren, l_paren_loc) << diag_build_finish;
  return false;
}

bool parser::expect_right_brace_and_consume(std::size_t l_brace_loc) {
  if (tok.is(token_kind::op_r_brace)) {
    consume_token();
    return true;
  }
  // There is currently no typo correction strategy for right brace.
  // error: expected '}'
  diag(err_expect, tok.get_start_location()) << '}' << diag_build_finish;
  // note: to match this '{'
  diag(note_match_l_brace, l_brace_loc) << diag_build_finish;
  return false;
}

std::size_t parser::expect_semi_and_consume(string_ref after, bool check_typo) {
  if (tok.is(token_kind::op_semi)) {
    std::size_t loc = tok.get_start_location();
    consume_token();
    return loc;
  }
  if (check_typo && maybe_typo(tok, token_kind::op_semi)) {
    this->diag(err_expect, tok.get_start_location(), tok.get_start_location())
        << ';'
        << _diag_engine.create_replacement(tok.get_start_location(),
                                           tok.get_end_location(),
                                           ";") << diag_build_finish;
    std::size_t loc = tok.get_start_location();
    consume_token();
    return loc;
  }
  // we can fixit here
  this->diag(err_expect_semi_after, tok.get_start_location(), tok.get_start_location())
      << after
      << _diag_engine.create_insertion_after_location(prev_tok_loc - 1, ";") // FIXME: should we check whether prev_tok_loc is not 0 here?
      << diag_build_finish;
  return prev_tok_loc;
}

bool parser::maybe_typo(token input, token_kind expected_tok) {
  // if we expected an operator
  // fix `.` or `:` to `;`
  if (expected_tok == token_kind::op_semi) {
    return input.get_data() == ":" || input.get_data() == ".";
  }
  // fix '.' to ','
  if (expected_tok == token_kind::op_comma) {
    return input.get_data() == ".";
  }
  // fix '\' to '/'
  if (expected_tok == token_kind::op_slash) {
    return input.get_data() == "\\";
  }
  // if we expected a keyword
  if (is_keyword(expected_tok)) {
    string_ref spelling = get_spelling(expected_tok);
    unsigned max_edit_distance = 3;
    auto distance = input.get_data().edit_distance(get_spelling(expected_tok), /* ignore_cases = */ true);
    return distance <= max_edit_distance && distance < input.get_length() && distance < spelling.size();
  }
  return false;
}

bool parser::skip_until(const std::vector<token_kind>& kinds,
                        bool stop_before_match, bool stop_before_semi) {
  bool _is_first_token_skipped = true;
  while (true) {
    for (token_kind kind : kinds) {
      if (tok.is(kind)) {
        if (!stop_before_match)
          consume_token();
        return true;
      }
    }
    switch (tok.get_kind()) {
      case token_kind::tk_eof:
        return false;
      case token_kind::op_l_paren:
        _consume_paren();
        skip_until(token_kind::op_r_paren);
        break;
      case token_kind::op_l_brace:
        _consume_brace();
        skip_until(token_kind::op_r_brace);
        break;
      case token_kind::op_r_paren:
        if (_paren_count && !_is_first_token_skipped)
          return false;
        _consume_paren();
        break;
      case token_kind::op_r_brace:
        if (_brace_count && !_is_first_token_skipped)
          return false;
        _consume_brace();
        break;
      case token_kind::op_semi:
        if (stop_before_semi)
          return false;
        [[fallthrough]];
      default:
        consume_token();
        break;
    }
    _is_first_token_skipped = false;
  }
}

parser::stmt_group parser::parse_program() {
  stmt_group result;
  while (tok.is_not(token_kind::tk_eof)) {
    result.emplace_back(parse_stmt());
  }
  return result;
}

stmt_result_t parser::parse_stmt() {
  switch (tok.get_kind()) {
    case token_kind::op_semi:
      return parse_empty_stmt();
    case token_kind::kw_origin:
    case token_kind::kw_scale:
    case token_kind::kw_rot:
    case token_kind::tk_identifier:
      if (next_token().is(token_kind::kw_is) || maybe_typo(next_token(), token_kind::kw_is))
        return parse_assignment_stmt();
      else
        return parse_expr_stmt();
    case token_kind::kw_for:
      return parse_for_stmt();
    default:
      return parse_expr_stmt();
  }
}

/**
 * AssignmentStatement
 *    OriginStatement
 *    ScaleStatement
 *    RotStatement
 *    IdAssignmentStatement
 *
 * OriginStatement
 *    'origin' 'is' expression
 *
 * ScaleStatement
 *    'scale' 'is' expression
 *
 * RotStatement
 *    'rot' 'is' expression
 *
 * IdAssignmentStatement
 *    identifier 'is' expression
 */
stmt_result_t parser::parse_assignment_stmt() {
  expr_result_t lhs = parse_expr();
  bool invalid = false;
  if (!lhs)
    invalid = true;
  else if (lhs->get_stmt_kind() != stmt::variable_expr_type) {
    invalid = true;
    diag(err_expect_variable, lhs->get_start_loc()) << diag_build_finish;
  }
  std::size_t is_loc = tok.get_start_location();
  auto result = expect_and_consume(token_kind::kw_is, err_expect, "is", "", true);
  assert(result);
  expr_result_t value = parse_expr();
  if (!value) {
    STMT_SKIP
    return stmt_error();
  }
  // We don't check the type of the expression here,
  // it will be done by the interpreter.
  // expect ';'
  std::size_t semi_loc = expect_semi_and_consume("statement");
  if (invalid)
    return stmt_error();
  return std::make_unique<assignment_stmt>(std::move(lhs), is_loc,
                                           std::move(value), semi_loc);
}

/**
 * ForStatement
 *    'for' VariableExpr
 *    ['from' expression](opt)
 *    'to' expression
 *    ['step' expression](opt)
 *    (Statement | StatementList)
 *
 * StatementList
 *    `{` Statement+ `}`
 */
stmt_result_t parser::parse_for_stmt() {
  assert(tok.is(token_kind::kw_for));
  bool invalid = false;
  std::size_t for_loc = tok.get_start_location();
  consume_token();  // eat 'for'

  // expect a VariableExpr
  // Currently, only variable expressions represented by an identifier are supported,
  // even `(t)` is invalid here.
  // Here we try to parse it into a general expression to find the wrong type of
  // expression provided by the user.
  expr_result_t _for_value_expr = parse_expr();
  if (!_for_value_expr)
    invalid = true;
  else if (_for_value_expr->get_stmt_kind() != stmt::variable_expr_type) {
    diag(err_expect_variable, _for_value_expr->get_start_loc()) << diag_build_finish;
    invalid = true;
    // continue to parse
  }
  // We don’t need to deal with the invalid expression here.

  // Parse the optional `from` clause
  // If the next token is 'from' or maybe typo of 'from'
  std::size_t from_loc = tok.get_start_location();
  expr_result_t from_expr = nullptr;
  if (tok.is(token_kind::kw_from) ||
      maybe_typo(tok, token_kind::kw_from)) {
    consume_token();  // eat `from`
    from_expr = parse_expr();
    if (!from_expr)
      invalid = true;
  }

  // Parse the 'to' clause
  std::size_t to_loc = tok.get_start_location();
  if (!expect_and_consume(token_kind::kw_to, err_expect, "to", "")) {
    STMT_SKIP
    return stmt_error();
  }
  expr_result_t to_expr = parse_expr();
  if (!to_expr)
    invalid = true;

  // parse the optional 'step' clause
  // We don't check the typo here because we can’t
  // mistake the expression for a step keyword.
  std::size_t step_loc = tok.get_start_location();
  expr_result_t step_expr = nullptr;
  if (tok.is(token_kind::kw_step)) {
    consume_token();
    step_expr = parse_expr();
    if (!step_expr)
      invalid = true;
  }

  // parse the body of `for`
  std::vector<stmt_result_t> for_body;
  if (tok.is(token_kind::op_l_brace)) {
    for_body = parse_stmt_list();
  } else {
    for_body.emplace_back(parse_stmt());
  }

  // generate `for` node
  if (invalid)
    return stmt_error();
  return std::make_unique<for_stmt>(for_loc, std::move(_for_value_expr),
                                    from_loc, std::move(from_expr),
                                    to_loc, std::move(to_expr),
                                    step_loc, std::move(step_expr),
                                    prev_tok_loc, std::move(for_body));
}

/**
 * EmptyStatement
 *    `;`
 */
stmt_result_t parser::parse_empty_stmt() {
  assert(tok.is(token_kind::op_semi));
  std::size_t loc = tok.get_start_location();
  consume_token();
  return std::make_unique<empty_stmt>(loc);
}

/**
 * ExprStatement
 *    Expr `;`
 */
stmt_result_t parser::parse_expr_stmt() {
  expr_result_t e = parse_expr();
  if (!e) {
    STMT_SKIP
    return stmt_error();
  }
  std::size_t semi_loc = expect_semi_and_consume("expression");
  return std::make_unique<expr_stmt>(std::move(e), semi_loc);
}

std::vector<stmt_result_t> parser::parse_stmt_list() {
  assert(tok.is(token_kind::op_l_brace));
  std::size_t l_brace_loc = tok.get_start_location();
  consume_token();
  std::vector<stmt_result_t> result;
  while (!tok.is_one_of(token_kind::op_r_brace, token_kind::tk_eof)) {
    stmt_result_t cur_stmt = parse_stmt();
    if (cur_stmt)
      result.emplace_back(std::move(cur_stmt));
  }
  expect_right_brace_and_consume(l_brace_loc);
  return result;
}

/**
 * NumberExpr
 *      token_kind::tk_constant
 */
expr_result_t parser::parse_constant_value() {
  assert(tok.is(token_kind::tk_constant));
  token value = tok;
  consume_token();  // move to the next token
  double result_value;
  string_ref data = value.get_data();
  try {
    result_value = std::stod(static_cast<std::string>(data));
  } catch(std::out_of_range&) {
    diag(err_constant_too_large, value.get_start_location(), value.get_end_location())
      << diag_build_finish;
    return expr_error();
  }

  bool has_float_point = std::find(data.begin(), data.end(), '.') != data.end();
  return std::make_unique<num_expr>(result_value,
                                    value.get_start_location(),
                                    value.get_end_location(),
                                    has_float_point);
}

std::string parser::extract_string_token_value(token& t) {
  assert(t.is(token_kind::tk_string));
  string_ref raw = t.get_data();
  std::string data = std::string(raw.begin() + 1, raw.end() - 1);
  std::string result;
  for (auto iter = data.begin(); iter != data.end(); ++iter) {
    if (*iter == '\\') {
      ++iter; // iter cannot be 'end()' here, because in this case
              // the lexer will treat it as token_kind:tk_unknown
      assert(iter != data.end());
      switch (*iter) {
        case '\'':
        case '"':
        case '?':
        case '\\':
          result += *iter;
          break;
        case 'a':
          result += '\a';
          break;
        case 'b':
          result += '\b';
          break;
        case 'f':
          result += '\f';
          break;
        case 'n':
          result += '\n';
          break;
        case 'r':
          result += '\r';
          break;
        case 't':
          result += '\t';
          break;
        case 'v':
          result += '\v';
          break;
        default:
          std::size_t loc = iter - data.begin() + t.get_start_location();
          diag(warn_unknown_escape, loc - 1, loc)
            << "\\" + std::string{ *iter } << diag_build_finish;
          result += *iter;
      }
    } else
      result += *iter;
  }
  return result;
}

/**
 * StringExpr
 *    tk_string+
 */
expr_result_t parser::parse_string_value() {
  assert(tok.is(token_kind::tk_string));
  std::size_t start_loc = tok.get_start_location();
  std::string result;
  while (tok.is(token_kind::tk_string)) {
    result += extract_string_token_value(tok);
    consume_token();
  }
  return std::make_unique<string_expr>(std::move(result), start_loc, prev_tok_loc);
}

/**
 * IdentifierExpr
 *    VariableExpr
 *    FunctionCallExpr
 *
 * VariableExpr
 *    Identifier
 *
 * FunctionCallExpr
 *    Identifier '(' expression (',' expression)* ')'
 *
 * Identifier
 *    tk_identifier
 *    kw_origin
 *    kw_scale
 *    kw_rot
 *    kw_draw
 *    kw_t
 */
expr_result_t parser::parse_identifier_expr() {
  if (next_token().is_not(token_kind::op_l_paren)) {
    return parse_variable_expr();
  } else {
    return parse_call_expr();
  }
}

expr_result_t parser::parse_call_expr() {
  token func_name_tok = tok;
  consume_token();  // eat the function name
  assert(tok.is(token_kind::op_l_paren));
  std::size_t l_paren_loc = tok.get_start_location();
  consume_token();  // eat the left paren of the param list

  call_expr::param_list_t param_expr;
  if (!tok.is_one_of(token_kind::op_r_paren, token_kind::tk_eof)) {
    // If we don't meet the eof and ')'
    // parse the param list
    bool _param_list_invalid = false;
    param_expr = parse_expr_list(_param_list_invalid);
    if (_param_list_invalid) {
      skip_until(token_kind::op_r_paren, /* stop_before_match = */false,
                 /* stop_before_semi = */true);
      return expr_error();
    }
  }
  std::size_t r_paren_loc = tok.get_start_location();
  if (!expect_right_paren_and_consume(l_paren_loc)) {
    skip_until(token_kind::op_r_paren, /* stop_before_match = */false,
               /* stop_before_semi = */true);
    return expr_error();
  }
  auto result = std::make_unique<call_expr>(get_token_spelling(func_name_tok),
                                            std::move(param_expr),
                                            func_name_tok.get_start_location(),
                                            l_paren_loc, r_paren_loc);
  return result;
}

expr_result_t parser::parse_variable_expr() {
  token cur = tok;
  consume_token();
  return std::make_unique<variable_expr>(get_token_spelling(cur),
                                         cur.get_start_location(),
                                         cur.get_end_location());
}

std::vector<expr_result_t> parser::parse_expr_list(bool& invalid) {
  std::vector<expr_result_t> result;
  while (true) {
    if (auto param = parse_expr()) {
      result.emplace_back(std::move(param));
    } else {
      // the expression of the single param is invalid,
      // so we skip to find the ',' or ')' to recover
      // from the error and find more errors
      skip_until(token_kind::op_comma, token_kind::op_r_paren,
          /* stop_before_match = */true, /* stop_before_semi = */true);
      invalid = true;
    }
    if (tok.is_not(token_kind::op_comma))
      break;
    consume_token();  // consume the ','
  }
  return result;
}

string_ref parser::get_token_spelling(token t) {
  if (t.is(token_kind::tk_identifier))
    return t.get_data();
  return get_spelling(t.get_kind());
}

/**
 * ParenExpr
 *    '(' expression (',' expression)* ')'
 */
expr_result_t parser::parse_paren_expr() {
  assert(tok.is(token_kind::op_l_paren));
  std::size_t l_paren_loc = tok.get_start_location();
  consume_token();  // eat '('
  bool invalid = false;
  std::vector<std::unique_ptr<expr>> result = parse_expr_list(invalid);
  // If there are some errors in the expression element,
  // the corresponding parse_* function will make a diag.
  if (invalid) {
    // lex until we meet a ')' and consume it
    skip_until(token_kind::op_r_paren, /* stop_before_match = */false,
               /* stop_before_semi = */true);
    return expr_error();
  }
  std::size_t r_paren_loc = tok.get_start_location();
  if (!expect_right_paren_and_consume(l_paren_loc)) {
    skip_until(token_kind::op_r_paren, /* stop_before_match = */false,
               /* stop_before_semi = */true);
    return expr_error();
  }
  // check whether it is a normal expression or a tuple
  assert(!result.empty());
  if (result.size() == 1)
    return std::move(result.front());
  return std::make_unique<tuple_expr>(std::move(result), l_paren_loc, r_paren_loc);
}

/**
 * Expr
 *    IdentifierExpr
 *    ConstantExpr
 *    StringExpr
 *    ParenExpr
 *    BinaryExpr
 *    UnaryExpr
 *
 * BinaryExpr
 *    Expr '+' Expr
 *    Expr '-' Expr
 *    Expr '*' Expr
 *    Expr '/' Expr
 *    Expr '**' Expr
 *
 * UnaryExpr
 *    '+' Expr
 *    '-' Expr
 */
expr_result_t parser::parse_expr() {
  std::vector<std::unique_ptr<expr>> _operand_stack;
  std::vector<std::tuple<token, int, bool>> _op_stack;  // the token of the operator, prec, is_binary_operator
  bool invalid = false;
  auto _combine_stack = [&_operand_stack, &_op_stack]() {
    token op_token = std::get<0>(_op_stack.back());
    if (std::get<2>(_op_stack.back())) {
      std::unique_ptr<expr> rhs = std::move(_operand_stack.back());
      _operand_stack.pop_back();
      std::unique_ptr<expr> lhs = std::move(_operand_stack.back());
      _operand_stack.pop_back();
      // binary operator
      assert(lhs && rhs);
      auto _binary = binary_expr::create_binary_op(op_token, std::move(lhs), std::move(rhs));
      _operand_stack.emplace_back(std::move(_binary));
    } else {
      std::unique_ptr<expr> lhs = std::move(_operand_stack.back());
      _operand_stack.pop_back();
      // unary operator
      assert(lhs);
      auto _unary = unary_expr::create_unary_op(op_token, std::move(lhs));
      _operand_stack.emplace_back(std::move(_unary));
    }
    _op_stack.pop_back();
  };
  bool expect_op = false;
  for (;;) {
    switch (tok.get_kind()) {
      case token_kind::kw_origin:
      case token_kind::kw_scale:
      case token_kind::kw_rot:
      case token_kind::kw_draw:
      case token_kind::kw_t:
      case token_kind::tk_identifier:
        if (!expect_op) {
          expr_result_t e = parse_identifier_expr();
          if (!e)
            invalid = true;
          _operand_stack.emplace_back(std::move(e));
          expect_op = true;
          break;
        } else
          goto finish;
      case token_kind::tk_constant:
        if (!expect_op) {
          expr_result_t e = parse_constant_value();
          if (!e)
            invalid = true;
          _operand_stack.emplace_back(std::move(e));
          expect_op = true;
          break;
        } else
          goto finish;
      case token_kind::tk_string:
        if (!expect_op) {
          expr_result_t e = parse_string_value();
          if (!e)
            invalid = true;
          _operand_stack.emplace_back(std::move(e));
          expect_op = true;
          break;
        } else
          goto finish;
      case token_kind::op_l_paren:
        if (!expect_op) {
          expr_result_t e = parse_paren_expr();
          if (!e)
            invalid = true;
          _operand_stack.emplace_back(std::move(e));
          expect_op = true;
          break;
        } else
          goto finish;
#define ARITH_OP(NAME) case token_kind::NAME:
#include <AST/OpKindDef.h>
      {
        auto [prec, right_assoc] = get_op_prec(tok.get_kind(), expect_op);
        // This is an invalid operator. For example, if we expect a unary
        // operator, but encounter an operator that cannot be a unary
        // operator, this will happen.
        if (prec < 0) {
          // We simply discard the operator and continue parsing to try to find more errors.
          // For example, if we meet `2 + * 3 +`, we will find 2 errors:
          //    2 + * 3 +
          //        ^
          //        `*` cannot be a unary operator
          //    2 + * 3 +
          //             ^
          //             expected expression
          diag(err_cannot_be_unary, tok.get_start_location())
            << get_spelling(tok.get_kind()) << diag_build_finish;
          invalid = true;
          consume_token();
          continue;
        }
        while (expect_op && !_op_stack.empty() &&
               (std::get<1>(_op_stack.back()) > prec ||
                (!right_assoc && std::get<1>(_op_stack.back()) == prec))) {
          _combine_stack();
        }
        _op_stack.emplace_back(tok, prec, expect_op);
        expect_op = false;
        consume_token();
        break;
      }
      default:
        goto finish;
    }
  }
finish:
  if (!expect_op) {
    // we still expect a primary expression
    diag(err_expect_expr, tok.get_start_location()) << diag_build_finish;
    // FIXME: Do we need error recovery here?
    return expr_error();
  } else if (invalid) {
    return expr_error();
  } else {
    while (!_op_stack.empty()) {
      _combine_stack();
    }
    assert(_operand_stack.size() == 1);
    return std::move(_operand_stack.front());
  }
}

std::pair<int, bool> parser::get_op_prec(token_kind op, bool is_binary) {
  if (is_binary) {
    switch (op) {
#define BIN_OP(NAME, OP, PREC, ASSOC, TOKEN_KIND) \
      case token_kind::TOKEN_KIND: return { PREC, ASSOC };
#include <AST/OpKindDef.h>
      default:
        return { -1, 0 };
    }
  } else {
    switch (op) {
#define UNARY_OP(NAME, OP, PREC, ASSOC, TOKEN_KIND) \
      case token_kind::TOKEN_KIND: return { PREC, ASSOC };
#include <AST/OpKindDef.h>
      default:
        return { -1, 0 };
    }
  }
}


INTERPRETER_NAMESPACE_END
