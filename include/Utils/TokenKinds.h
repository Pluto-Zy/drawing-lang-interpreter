/**
 * This file defines the @code{token_kind} enumerators.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_TOKENKINDS_H
#define DRAWING_LANG_INTERPRETER_TOKENKINDS_H

#include "def.h"

INTERPRETER_NAMESPACE_BEGIN

enum class token_kind : unsigned char {
  tk_unknown,
  tk_eof,
  tk_identifier,  // abc123
  tk_constant,    // 123.4
  // keywords
  kw_origin,
  kw_scale,
  kw_rot,
  kw_is,
  kw_to,
  kw_step,
  kw_draw,
  kw_for,
  kw_from,
  kw_t,
  // operators
  op_semi,      // ;
  op_l_paren,   // (
  op_r_paren,   // )
  op_comma,     // ,
  op_plus,      // +
  op_minus,     // -
  op_star,      // *
  op_slash,     // /
  op_star_star, // **
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_TOKENKINDS_H
