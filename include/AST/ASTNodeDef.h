/**
 * This file defines all the AST nodes inherited from @code{stmt}.
 * The macro @code{STMT} is used to define @code{stmt_visitor}.
 *
 * For example, @code{STMT(D, B)} means there is a class named @code{B}
 * which is @code{stmt} or inherited from @code{stmt}, and there is a class
 * name @code{D} whose direct base class is @code{B}.
 *
 * @author 19030500131 zy
 */
#ifndef STMT
#define STMT(CLASS, PARENT)
#endif

STMT(empty_stmt, stmt)
STMT(operand_stmt, stmt)
STMT(assignment_stmt, operand_stmt)
STMT(for_stmt, operand_stmt)
STMT(expr_stmt, operand_stmt)

STMT(expr, stmt)
STMT(binary_expr, expr)
STMT(unary_expr, expr)
STMT(variable_expr, expr)
STMT(num_expr, expr)
STMT(string_expr, expr)
STMT(tuple_expr, expr)
STMT(call_expr, expr)

#undef STMT
