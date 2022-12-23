/**
 * This file defines all the arithmetic operators in the language.
 *
 * @author 19030500131 zy
 */
#ifndef OP_KIND
#define OP_KIND(NAME, OPERATOR, OPERAND, PRECEDENCE, ASSOC, TOKEN_KIND)
#endif

#ifndef BIN_OP
#define BIN_OP(NAME, OP, PRECEDENCE, ASSOC, TOKEN_KIND) OP_KIND(NAME, OP, 2, PRECEDENCE, ASSOC, TOKEN_KIND)
#endif

#ifndef UNARY_OP
#define UNARY_OP(NAME, OP, PRECEDENCE, ASSOC, TOKEN_KIND) OP_KIND(NAME, OP, 1, PRECEDENCE, ASSOC, TOKEN_KIND)
#endif

#ifndef ARITH_OP
#define ARITH_OP(TOKEN_KIND)
#endif

ARITH_OP(op_plus)
ARITH_OP(op_minus)
ARITH_OP(op_star)
ARITH_OP(op_slash)
ARITH_OP(op_star_star)

BIN_OP(add, +, 10, 0, op_plus)
BIN_OP(sub, -, 10, 0, op_minus)
BIN_OP(mul, *, 20, 0, op_star)
BIN_OP(div, /, 20, 0, op_slash)
UNARY_OP(plus, +, 30, 1, op_plus)
UNARY_OP(minus, -, 30, 1, op_minus)
BIN_OP(pow, **, 40, 1, op_star_star)

#undef OP_KIND
#undef BIN_OP
#undef UNARY_OP
#undef ARITH_OP
