#include <AST/Expr.h>
#include <unordered_map>
#include <Lex/Token.h>

INTERPRETER_NAMESPACE_BEGIN

string_ref binary_expr::get_op_str(op_kind kind) {
  switch (kind) {
    default:
      return "";
#define BIN_OP(NAME, OP, PRECEDENCE, ASSOC, TOKEN) case bo_##NAME: return #OP;
#include <AST/OpKindDef.h>
  }
}

binary_expr::op_kind binary_expr::token_kind_to_op_kind(token_kind kind) {
  switch (kind) {
#define BIN_OP(NAME, OP, PRECEDENCE, ASSOC, TOKEN) case token_kind::TOKEN: return bo_##NAME;
#include <AST/OpKindDef.h>
    default:
      return bo_unknown;
  }
}

std::unique_ptr<binary_expr>
binary_expr::create_binary_op(const token& tok, expr_result_t lhs, expr_result_t rhs) {
  // we cannot use make_unique here because the constructor is protected
  return std::unique_ptr<binary_expr>(
      new binary_expr(token_kind_to_op_kind(tok.get_kind()),
                      tok.get_start_location(), std::move(lhs), std::move(rhs)));
}

string_ref unary_expr::get_op_str(unary_expr::op_kind kind) {
  switch (kind) {
    default:
      return "";
#define UNARY_OP(NAME, OP, PREC, ASSOC, TOKEN) case uo_##NAME: return #OP;
#include <AST/OpKindDef.h>
  }
}

unary_expr::op_kind unary_expr::token_kind_to_op_kind(token_kind kind) {
  switch (kind) {
#define UNARY_OP(NAME, OP, PREC, ASSOC, TOKEN) case token_kind::TOKEN: return uo_##NAME;
#include <AST/OpKindDef.h>
    default:
      return uo_unknown;
  }
}

std::unique_ptr<unary_expr>
unary_expr::create_unary_op(const token& tok, expr_result_t operand) {
  // we cannot use make_unique here because the constructor is protected
  return std::unique_ptr<unary_expr>(
      new unary_expr(token_kind_to_op_kind(tok.get_kind()),
                     tok.get_start_location(), std::move(operand)));
}

INTERPRETER_NAMESPACE_END
