#include <Lex/Lexer.h>

#define CUR_IS(ch) (!at_end() && *_buf_cur == (ch))
#define CUR_IS_NOT(ch) (!at_end() && *_buf_cur != (ch))

INTERPRETER_NAMESPACE_BEGIN

lexer::table_t lexer::kw_table({
#define keyword(spelling) {#spelling, token_kind::kw_##spelling},
#include "Lex/KeywordDef.h"
#undef keyword
}, /*bucket_count=*/10, &hash_value_lower_case,
[](string_ref lhs, string_ref rhs) -> bool {
  return lhs.compare_insensitive(rhs) == 0;
});

void lexer::_skip_white_space() {
  for (; !at_end() && std::isspace(*_buf_cur); ++_buf_cur)
    ;
}

void lexer::_skip_line_comment() {
  for (; CUR_IS_NOT('\n'); ++_buf_cur)
    ;
  if (!at_end())
    ++_buf_cur;
}

void lexer::_form_token_from_range(token &result, const char *beg, token_kind kind) const {
  result.set_kind(kind);
  result.set_data({beg, static_cast<string_ref::size_type>(_buf_cur - beg)});
  result.set_location(beg - _buf_beg);
}

/**
 * Lex the remainder of a floating point constant.
 * `_cur_ptr` points the second character of the constant.
 *
 * const_id := digit+("." digit*)?
 */
void lexer::_lex_float_constant(token &result, const char *start_ptr) {
  for (; !at_end() && std::isdigit(*_buf_cur); ++_buf_cur)
    ;
  // now *_buf_cur is not a digit
  if (CUR_IS('.')) {
    ++_buf_cur;
    for (; !at_end() && std::isdigit(*_buf_cur); ++_buf_cur)
      ;
  }
  _form_token_from_range(result, start_ptr, token_kind::tk_constant);
}

/**
 * Lex the remainder of an identifier.
 * `_cur_ptr` points the second character of the identifier.
 *
 * id := letter+(letter|digit)*
 */
void lexer::_lex_identifier(token& result, const char* start_ptr) {
  for (; !at_end() && std::isalnum(*_buf_cur) || *_buf_cur == '_'; ++_buf_cur)
    ;
  _form_token_from_range(result, start_ptr, token_kind::tk_identifier);
  // check whether it is a keyword
  if (auto iter = kw_table.find(result.get_data()); iter != kw_table.end()) {
    result.set_kind(iter->second);
  }
}

void lexer::_lex_impl(token &result) {
  if (!_buf_cur)
    return;
Restart:
  const char* start_token_ptr = _buf_cur;
  token_kind kind;
  if (at_end()) {
    result.set_kind(token_kind::tk_eof);
    result.set_location(get_current_loc());
    return;
  }
  switch(*_buf_cur++) {
    // null character
    case 0:
      if (at_end()) {
        kind = token_kind::tk_eof;
        break;
      }
      else {
        diag(null_in_file, get_current_loc() - 1) << diag_build_finish;
        goto Restart;   // We got an invalid token, so we try again.
      }
    case '\r': case '\n': case ' ':
    case '\t': case '\f': case '\v':
      _skip_white_space();
      goto Restart;
    // number constant
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      _lex_float_constant(result, start_token_ptr);
      return;   // cannot use `break` here
    // identifiers
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
    case 'v': case 'w': case 'x': case 'y': case 'z':
    case '_':
      _lex_identifier(result, start_token_ptr);
      return;   // cannot use `break` here
    case '(':
      kind = token_kind::op_l_paren;
      break;
    case ')':
      kind = token_kind::op_r_paren;
      break;
    case '{':
      kind = token_kind::op_l_brace;
      break;
    case '}':
      kind = token_kind::op_r_brace;
      break;
    case '*':
      if (CUR_IS('*')) {
        ++_buf_cur;
        kind = token_kind::op_star_star;
      } else
        kind = token_kind::op_star;
      break;
    case '+':
      kind = token_kind::op_plus;
      break;
    case '-':
      // comment
      if (CUR_IS('-')) {
        _skip_line_comment();
        goto Restart;
      }
      kind = token_kind::op_minus;
      break;
    case '/':
      // comment
      if (CUR_IS('/')) {
        _skip_line_comment();
        goto Restart;
      }
      kind = token_kind::op_slash;
      break;
    case ';':
      kind = token_kind::op_semi;
      break;
    case ',':
      kind = token_kind::op_comma;
      break;
    default:
      kind = token_kind::tk_unknown;
      diag(err_unknown_char, get_current_loc() - 1) << _buf_cur[-1] << diag_build_finish;
      break;
  }
  _form_token_from_range(result, start_token_ptr, kind);
}

void lexer::lex_and_consume(token& result) {
  if (!_token_cache_list.empty())
    _token_cache_list.pop_front();
  if (_token_cache_list.empty()) {
    _lex_impl(result);
    _token_cache_list.push_back(result);
  } else {
    result = _token_cache_list.front();
  }
}

token lexer::lex_and_consume() {
  token result;
  lex_and_consume(result);
  return result;
}

void lexer::_lex_n_and_cache(std::size_t count) {
  while (count--) {
    token result;
    _lex_impl(result);
    _token_cache_list.emplace_back(std::move(result));
  }
}

void lexer::look_ahead(token& result, std::ptrdiff_t count) {
  if (!count)
    return;
  if (_token_cache_list.size() < static_cast<std::size_t>(count) + 1) {
    _lex_n_and_cache(count + 1 - _token_cache_list.size());
  }
  result = *std::next(_token_cache_list.begin(), count);
}

token lexer::look_ahead(std::ptrdiff_t count) {
  token result;
  look_ahead(result, count);
  return result;
}

void lexer::next_token(token& result) {
  look_ahead(result, 1);
}

token lexer::next_token() {
  token result;
  next_token(result);
  return result;
}

void lexer::consume() {
  if (!_token_cache_list.empty())
    _token_cache_list.pop_front();
  else
    (void)lex_and_consume();
}

void lexer::lex_until(token_kind kind) {
  for (; !next_token().is_one_of(token_kind::tk_eof, kind); consume())
    ;
}

void lexer::lex_until_and_consume(token_kind kind) {
  while (!lex_and_consume().is_one_of(token_kind::tk_eof, kind))
    ;
}

void lexer::lex_until_eol() {
  // FIXME: Under the current rules, the token will not contain a newline character,
  //  so we only need to read the position before and after the token.
  //  But maybe we need a better algorithm. If we discard all caches and
  //  then start lexing from the beginning, this may cause the tokens
  //  "put back" by the user to also be discarded.
  auto _has_new_line_checker = [this](std::size_t beg, std::size_t end) -> bool {
    for (; beg != end; ++beg) {
      if (_buf_beg[beg] == '\n')
        return true;
    }
    return false;
  };
  if (_token_cache_list.size() > 1) {
    for (auto cur = _token_cache_list.begin(), prev = cur++; cur != _token_cache_list.end(); prev = cur++) {
      if (_has_new_line_checker(prev->get_end_location(), cur->get_start_location())) {
        // Regenerate the prev token
        prev->set_location(prev->get_end_location());
        prev->set_data(string_ref(_buf_beg + prev->get_start_location(),
                                  cur->get_start_location() - prev->get_start_location()));
        prev->set_kind(token_kind::tk_unknown);
        return;
      }
      cur = _token_cache_list.erase(prev);
    }
  } else if (!_token_cache_list.empty()) {
    if (_has_new_line_checker(_token_cache_list.front().get_end_location(),
                              get_current_loc())) {
      // Regenerate the prev token
      token& prev = _token_cache_list.front();
      prev.set_location(prev.get_end_location());
      prev.set_data(string_ref(_buf_beg + prev.get_start_location(),
                               get_current_loc() - prev.get_start_location()));
      prev.set_kind(token_kind::tk_unknown);
      return;
    }
  }
  _token_cache_list.clear();
  for (; CUR_IS_NOT('\n'); ++_buf_cur)
    ;
  if (!at_end())
    ++_buf_cur;
  // Regenerate the prev token
  token _newline_last;
  _form_token_from_range(_newline_last, _buf_cur - 1, token_kind::tk_unknown);
  _token_cache_list.push_back(std::move(_newline_last));
}

INTERPRETER_NAMESPACE_END
