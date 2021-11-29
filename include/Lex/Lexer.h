/**
 * This file defines the @code{lexer} interface.
 *
 * The lexer will read from a source file represented by @code{string_ref},
 * and then split the character sequence into tokens according to certain rules.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_LEXER_H
#define DRAWING_LANG_INTERPRETER_LEXER_H

#include <Diagnostic/DiagEngine.h>
#include <Utils/FileManager.h>
#include "Token.h"
#include <list>
#include <unordered_map>
#include <functional>

INTERPRETER_NAMESPACE_BEGIN

class lexer {
public:
  lexer(const char* file_begin, const char* file_end, diag_engine& engine)
    : _buf_beg(file_begin), _buf_end(file_end), _buf_cur(_buf_beg),
      _diag_engine(engine), _token_cache_list{token()} { }

  lexer(const file_manager* file_manager, diag_engine& diag);

  bool at_end() const { return _buf_cur == _buf_end; }
  std::size_t get_current_loc() const { return _buf_cur - _buf_beg; }
  diag_engine& get_diag_engine() const { return _diag_engine; }

  /**
   * Returns the next token and discards it.
   * If there is no cached token, the new token will be parsed from the file.
   */
  void lex_and_consume(token& result);
  token lex_and_consume();
  /**
   * Returns the nth token and saves it to the cache list.
   */
  void look_ahead(token& result, std::ptrdiff_t count);
  token look_ahead(std::ptrdiff_t count);
  /**
   * Returns the next token and saves it to the cache list.
   */
  void next_token(token& result);
  token next_token();
  /**
   * Consumes the next token.
   */
  void consume();
  /**
   * Keep reading the next token until the lexer encounters
   * a token of type @code{kind}. The lexer will not consume the token.
   */
  void lex_until(token_kind kind);
  /**
   * Keep reading the next token until the lexer encounters
   * a token of type @code{kind}. The lexer will consume the token.
   */
  void lex_until_and_consume(token_kind kind);
  /**
   * Since there is no `token_kind` used to represent the end of a line,
   * interfaces such as `next_token` cannot be used directly.
   * We choose to look for newline character in the source character sequence,
   * but the unit test tells us that we must pay attention to the tokens
   * cached in `_token_cache_list`.
   * We can clear the cache list before looking up, but in fact,
   * the "newest token" the user can see is the first token in the cache list (if any),
   * and blindly clearing the cache list will cause the user to lose the
   * tokens that have not been consumed. So we should start looking up from the cache list.
   */
  void lex_until_eol();
private:
  /**
   * The start of the file buffer.
   */
  const char* _buf_beg;
  /**
   * The end of the file buffer.
   */
  const char* _buf_end;
  /**
   * A pointer to the character currently being processed.
   */
  const char* _buf_cur;
  /**
   * The diagnostic engine used to report errors.
   */
  diag_engine& _diag_engine;
  /**
   * Caches all tokens that have been lexed but not consumed.
   * In particular, the most recently discarded token will
   * be stored at the head of the list.
   */
  std::list<token> _token_cache_list;

  using table_t =
      std::unordered_map<string_ref, token_kind,
                         std::function<std::size_t(string_ref)>,
                         std::function<bool(string_ref, string_ref)>>;
  /**
   * Saves the keyword and their spellings.
   */
  static table_t kw_table;

  /**
   * Helper function used to make a diagnostic message
   */
  template<class... LocTy>
  diag_builder diag(diag_id message, LocTy... locs) {
    return _diag_engine.create_diag(message, locs...);
  }
  /**
   * Internal interface to lex a token.
   */
  void _lex_impl(token& result);
  /**
   * Modify the token information according to the given range.
   * The range is specified as [beg, _buf_cur).
   */
  void _form_token_from_range(token& result, const char* beg, token_kind kind) const;

  void _skip_white_space();
  void _skip_line_comment();

  void _lex_float_constant(token& result, const char* start_ptr);
  void _lex_identifier(token& result, const char* start_ptr);

  void _lex_n_and_cache(std::size_t count);
};

inline lexer::lexer(const drawing::file_manager *file_manager, diag_engine& diag)
  : _diag_engine(diag), _token_cache_list{token()} {
  if (file_manager) {
    _buf_cur = _buf_beg = file_manager->get_file_buf_begin();
    _buf_end = file_manager->get_file_buf_end();
  }
}

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_LEXER_H
