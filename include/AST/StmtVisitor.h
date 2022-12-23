/**
 * This file defines the @code{stmt_visitor} class template.
 */
#ifndef DRAWING_LANG_INTERPRETER_STMTVISITOR_H
#define DRAWING_LANG_INTERPRETER_STMTVISITOR_H

#include "Stmt.h"
#include "Expr.h"

INTERPRETER_NAMESPACE_BEGIN

/**
 * This class template implements a simple visitor for @code{stmt}
 * subclasses. Since @code{expr} derives from @code{stmt}, this
 * also includes support for visiting expressions.
 *
 * If you want to visit specific type of node, you can define a
 * class and use the visitor as the base class. For example,
 *
 *    class my_visitor : public stmt_visitor<my_visitor>
 *
 * Then implements specific member function. For example, if you
 * want to visit all the variable expression, you should implement
 *
 *    RetTy visit_expr_stmt(expr_stmt* e)
 *
 * The return type of the method can be defined in the template
 * argument:
 *
 *    class my_visitor : public stmt_visitor<my_visitor, int> {
 *    public:
 *      int visit_expr_stmt(expr_stmt* e) { ... }
 *    };
 *
 * If you want to add extra parameters to the visit_* method, you
 * should also provide them as the template arguments:
 *
 *    class my_visitor : public stmt_visitor<my_visitor, int, double> {
 *    public:
 *      int visit_expr_stmt(expr_stmt* e, double) { ... }
 *    };
 *
 * You can visit all kinds of nodes defined in @file{ASTNodeDef.h},
 * in addition, you can also access all types of binary operators
 * and unary operators, for example, visit_binary_add_op.
 *
 * You can start a visitor by calling its @code{visit} method and
 * provide a @code{stmt*} pointer with extra arguments (if any).
 * The visitor does not automatically traverse the syntax tree,
 * so you need to manually call the @code{visit} method on the
 * child nodes of a specific node.
 *
 * There are default implementation of every visit_* method.
 * Except for @code{visit_stmt}, the default implementation of the
 * visit_* method of other nodes is to call the corresponding
 * visit_* method of its parent node, which depends on the
 * definition of the node's direct base class in ASTNodeDef.h.
 */
template<template<class> class PtrTrait, class Derived,
         class RetTy = void, class... ParamTys>
class stmt_visitor_base {
public:
// convert CLASS to CLASS* or const CLASS*
#define PTR(CLASS) typename PtrTrait<CLASS>::type
// call corresponding visit_NAME function and convert
// the pointer to `stmt` into a pointer or const pointer
// to `CLASS`.
#define DISPATCH(NAME, CLASS)                                       \
  return static_cast<Derived*>(this)->visit_ ## NAME(               \
    static_cast<PTR(CLASS)>(s), std::forward<ParamTys>(p)...)

  RetTy visit(PTR(stmt) s, ParamTys... p) {
    if (s->get_stmt_kind() == stmt::binary_expr_type) {
      PTR(binary_expr) _binary_op = static_cast<PTR(binary_expr)>(s);
      switch (_binary_op->get_op_kind()) {
#define BIN_OP(NAME, OP, PREC, ASSOC, TOKEN) \
  case binary_expr::bo_##NAME: DISPATCH(binary_##NAME##_op, binary_expr);
#include "OpKindDef.h"
        default:
          assert(false);
      }
    } else if (s->get_stmt_kind() == stmt::unary_expr_type) {
      PTR(unary_expr) _unary_op = static_cast<PTR(unary_expr)>(s);
      switch (_unary_op->get_op_kind()) {
#define UNARY_OP(NAME, OP, PREC, ASSOC, TOKEN) \
  case unary_expr::uo_##NAME: DISPATCH(unary_##NAME##_op, unary_expr);
#include "OpKindDef.h"
        default:
          assert(false);
      }
    }
    switch (s->get_stmt_kind()) {
#define STMT(CLASS, PARENT) \
  case stmt::CLASS##_type: DISPATCH(CLASS, CLASS);
#include "ASTNodeDef.h"
      default:
        assert(false);
        return RetTy();
    }
  }

  // provide default implementation of visit_*
#define STMT(CLASS, PARENT) \
  RetTy visit_ ## CLASS(PTR(CLASS) s, ParamTys... p) \
  { DISPATCH(PARENT, PARENT); }
#include "ASTNodeDef.h"

  // provide default implementation of visit_binary_*_op
#define BIN_OP(NAME, OP, PREC, ASSOC, TOKEN) \
  RetTy visit_binary_ ## NAME ## _op(PTR(binary_expr) s, ParamTys... p) \
  { DISPATCH(binary_expr, binary_expr); }
#include "OpKindDef.h"

  // provide default implementation of visit_unary_*_op
#define UNARY_OP(NAME, OP, PREC, ASSOC, TOKEN) \
  RetTy visit_unary_ ## NAME ## _op(PTR(unary_expr) s, ParamTys... p) \
  { DISPATCH(unary_expr, unary_expr); }
#include "OpKindDef.h"

  RetTy visit_stmt(PTR(stmt) s, ParamTys... p) { return RetTy(); }
};

template<class Derived, class RetTy = void, class... ParamTys>
class stmt_visitor
    : public stmt_visitor_base<std::add_pointer, Derived, RetTy, ParamTys...> { };

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_STMTVISITOR_H
