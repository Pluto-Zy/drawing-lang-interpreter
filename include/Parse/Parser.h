/**
 * This file defines the parser of the language.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_PARSER_H
#define DRAWING_LANG_INTERPRETER_PARSER_H

#include <Lex/Lexer.h>
#include <vector>
#include <AST/Stmt.h>
#include <AST/Expr.h>

INTERPRETER_NAMESPACE_BEGIN

class parser {
  /**
   * The lexer.
   */
  lexer& _lexer;
  /**
   * The current token we are peeking ahead.
   */
  token tok;
  /**
   * The location of the last character of the token we
   * previously consumed. This token is used for diagnostics
   * where we expected to see a token following another token.
   */
  std::size_t prev_tok_loc;
  /**
   * Used to report diag message.
   */
  diag_engine& _diag_engine;

  unsigned short _paren_count, _brace_count;
  /**
   * Helper function used to make a diagnostic message
   */
  template<class... LocTy>
  diag_builder diag(diag_id message, LocTy... locs) {
    return _diag_engine.create_diag(message, locs...);
  }
public:
  explicit parser(lexer& _l);

  /**
   * Consumes the current token and lex the next one.
   * @return the location of the consumed token
   */
  std::size_t consume_token();
private:
  /**
   * Returns true if the cur token is '(' or ')'.
   */
  [[nodiscard]] bool _is_token_paren() const {
    return tok.is_one_of(token_kind::op_l_paren, token_kind::op_r_paren);
  }
  /**
   * Return true if the cur token is '{' or '}'.
   */
  [[nodiscard]] bool _is_token_brace() const {
    return tok.is_one_of(token_kind::op_l_brace, token_kind::op_r_brace);
  }
  /**
   * This consume method keeps the paren count up-to-date.
   */
  std::size_t _consume_paren();
  /**
   * This consume method keeps the brace count up-to-date.
   */
  std::size_t _consume_brace();

  /**
   * Peeks ahead one token and returns it without
   * consuming it.
   */
  token next_token();

  /**
   * Reads tokens until we get to the specified token, then consume it
   * (if @param{stop_before_match} is @code{false}).
   *
   * @return @code{true} if it finds the specified token, @code{false} otherwise.
   */
  bool skip_until(const std::vector<token_kind>& kinds,
                  bool stop_before_match = false,
                  bool stop_before_semi = false);
  bool skip_until(token_kind kind, bool stop_before_match = false, bool stop_before_semi = false) {
    return skip_until(std::vector<token_kind>{ kind }, stop_before_match, stop_before_semi);
  }
  bool skip_until(token_kind k1, token_kind k2, bool stop_before_match = false, bool stop_before_semi = false) {
    return skip_until({k1, k2}, stop_before_match, stop_before_semi);
  }

  /**
   * The parser expects the @param{expected_tok} is
   * next in the input. If so, it is consumed and @code{true}
   * is returned.
   *
   * If the next token is not the expected kind, the parser
   * will report @param{diag}. The general form of the reported
   * diagnostic information is
   *
   *    expected 'is' after 'origin'
   *
   * or
   *
   *    unknown identifier 'steq'; did you mean 'step'?
   *
   * If the parser finds that it encounters a typo (if @param{check_typo}
   * is @code{true}), it will create a hint to replace it, and assume
   * that the next token is the corrected token to continue parsing
   * (return @code{true}). Otherwise, the parser will return @code{false},
   * and the caller must adopt an appropriate error recovery strategy.
   *
   * @param expected_tok The expected type of the next token.
   *
   * @param diag Diagnostic information reported when the next
   * token does not match.
   *
   * @param expected Expected token spelling, used for diagnosis.
   *
   * @param after The spelling of the previous token, used for diagnosis.
   *
   * @param check_typo Whether to check for typo.
   *
   * @return Whether the next token meets the expectation.
   */
  bool expect_and_consume(token_kind expected_tok,
                          diag_id diag, string_ref expected,
                          string_ref after, bool check_typo = true);

  /**
   * Returns @code{true} if the next token is ')' and then consumes it.
   * Otherwise it will make a unique diag and return @code{false}.
   */
  bool expect_right_paren_and_consume(std::size_t l_paren_loc);
  /**
   * Returns @code{true} if the next token is '}' and then consumes it.
   * Otherwise it will make a unique diag and return @code{false}.
   */
  bool expect_right_brace_and_consume(std::size_t l_brace_loc);
  /**
   * If the next token is';', consume it. Otherwise, check whether
   * the next token is a spelling error, if it is, create a fixit
   * hint to replace the token, otherwise create a fixit hint to
   * insert the token.
   *
   * Since the semicolon always appears at the end of a statement,
   * if the semicolon is not matched correctly, we can always
   * create a fixit hint and let the parsing continue.
   *
   * Returns the location of the (inserted or replaced) semicolon.
   */
  std::size_t expect_semi_and_consume(string_ref after, bool check_typo = true);
private:
  /**
   * Check the input for possible typos according to the
   * expected token type.
   */
  static bool maybe_typo(token input, token_kind expected_tok);

  /**
   * Returns the spelling of @param{t}. If @code{t} is an identifier,
   * it will return the data of the token, otherwise return the lower-case
   * spelling of the token(e.g. token_kind::kw_origin returns "origin").
   */
  static string_ref get_token_spelling(token t);
public:
  using stmt_group = std::vector<stmt_result_t>;
  stmt_group parse_program();
  //===------------------------- statements --------------------------===//
  stmt_result_t parse_stmt();
  stmt_result_t parse_empty_stmt();
  stmt_result_t parse_assignment_stmt();
  stmt_result_t parse_for_stmt();
  stmt_result_t parse_expr_stmt();
  // Do not need the bool param because if there are some errors
  // in some statement, the interpreter can simply skip them.
  std::vector<stmt_result_t> parse_stmt_list();

  //===------------------------- expressions -------------------------===//
public:
  expr_result_t parse_constant_value();
  expr_result_t parse_identifier_expr();
  expr_result_t parse_variable_expr();
  expr_result_t parse_string_value();
  std::vector<expr_result_t> parse_expr_list(bool& invalid);
  expr_result_t parse_call_expr();
  expr_result_t parse_paren_expr();
  expr_result_t parse_expr();
private:
  /**
   * Returns the precedence (@code{first}) and the associativity (@code{second})
   * of the @param{op}. If the operator is invalid, its precedence will be less
   * than 0. If the operator is valid and left associative, the @code{second}
   * member will be @code{true}.
   */
  static std::pair<int, bool> get_op_prec(token_kind op, bool is_binary);
  /**
   * Extracts the actual string in the string token,
   * and processes the escape character.
   */
  std::string extract_string_token_value(token& t);
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_PARSER_H
