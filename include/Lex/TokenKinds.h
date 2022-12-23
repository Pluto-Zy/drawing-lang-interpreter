/**
 * This file defines the @code{token_kind} enumerators.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_TOKENKINDS_H
#define DRAWING_LANG_INTERPRETER_TOKENKINDS_H

#include <Utils/def.h>

INTERPRETER_NAMESPACE_BEGIN

enum class token_kind : unsigned char {
  tk_unknown,
  tk_eof,
  tk_identifier,  // abc123
  tk_constant,    // 123.4
  tk_string,      // "abc"

  // keywords
#define keyword(spelling) kw_##spelling,
#include "KeywordDef.h"
#undef keyword

  // operators
#define op(name, spelling) op_##name,
#include "OpDef.h"
#undef op
};

constexpr bool is_keyword(token_kind kind) {
#define keyword(spelling) || kind == token_kind::kw_##spelling
  return false
#include "KeywordDef.h"
  ;
#undef keyword
}

constexpr bool is_operator(token_kind kind) {
#define op(name, spelling) || kind == token_kind::op_##name
  return false
#include "OpDef.h"
  ;
#undef op
}

constexpr string_ref get_spelling(token_kind kind) {
  switch (kind) {
#define keyword(spelling) case token_kind::kw_##spelling: return #spelling;
#define op(name, spelling) case token_kind::op_##name: return spelling;
#include "KeywordDef.h"
#include "OpDef.h"

    default:
      return "";
  }
}

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_TOKENKINDS_H
