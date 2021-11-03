/**
 * This file defines the @code{Token} interface.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_TOKEN_H
#define DRAWING_LANG_INTERPRETER_TOKEN_H

#include "Utils/def.h"
#include "Utils/TokenKinds.h"
#include "Utils/StringRef.h"

INTERPRETER_NAMESPACE_BEGIN

class Token {
private:
  /**
   * The location of the token.
   * Represented by the offset of the first character of
   * the token in the file.
   */
  std::size_t _tok_loc;
  /**
   * The kind of the token.
   * Defined in Utils/TokenKinds.h
   */
  token_kind _kind;
  /**
   * The context of the token, if it refers to an identifier
   * or a constant.
   */
  string_ref _data;
public:
  Token() : _tok_loc(0), _kind(token_kind::tk_unknown), _data() { }

  token_kind get_kind() const { return _kind; }
  void set_kind(token_kind kind) { _kind = kind; }
  std::size_t get_location() const { return _tok_loc; }
  void set_location(std::size_t loc) { _tok_loc = loc; }
  string_ref get_data() const { return _data; }
  void set_data(string_ref data) { _data = std::move(data); }

  bool is(token_kind kind) const { return _kind == kind; }
  bool is_not(token_kind kind) const { return _kind != kind; }
  template<class... Ts, std::enable_if_t<sizeof...(Ts) > 0, int> = 0>
  bool is_one_of(Ts... kinds) const {
    return is(kinds) || ...;
  }
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_TOKEN_H
