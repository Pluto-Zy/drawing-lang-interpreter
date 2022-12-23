#include "ParserTest.h"
#include <AST/StmtVisitor.h>
#include <Sema/Sema.h>
#include <sstream>

INTERPRETER_NAMESPACE_BEGIN

namespace {
class expr_postfix_maker : public stmt_visitor<expr_postfix_maker> {
  std::string result;
public:
  void visit_binary_expr(binary_expr* e) {
    assert(e);
    visit(e->get_lhs());
    visit(e->get_rhs());
    result += static_cast<std::string>(e->get_op_str()) + ' ';
  }
  void visit_unary_expr(unary_expr* e) {
    assert(e);
    visit(e->get_operand());
    result += static_cast<std::string>(e->get_op_str()) + ' ';
  }
  void visit_variable_expr(variable_expr* e) {
    assert(e);
    result += static_cast<std::string>(e->get_name()) + ' ';
  }
  void visit_num_expr(num_expr* e) {
    assert(e);
    std::stringstream s;
    s.precision(15);
    s << e->get_value();
    result += s.str() + ' ';
  }
  void visit_tuple_expr(tuple_expr* e) {
    assert(e);
    result += '(';
    for (auto iter = e->elem_begin(); iter != e->elem_end(); ++iter) {
      visit(iter->get());
      result += ", ";
    }
    result += ") ";
  }
  void visit_call_expr(call_expr* e) {
    assert(e);
    result += static_cast<std::string>(e->get_func_name()) +
              '(';
    for (auto iter = e->param_begin(); iter != e->param_end(); ++iter) {
      visit(iter->get());
      result += ", ";
    }
    result += ") ";
  }
  void visit_string_expr(string_expr* e) {
    assert(e);
    result += e->get_value() + ' ';
  }

  std::string take_result() && { return std::move(result); }
};

std::string parse_expr_code(expr_result_t e) {
  if (!e)
    return "error ast";
  expr_postfix_maker maker;
  maker.visit(e.get());
  return std::move(maker).take_result();
}
}

#define PARSE_CODE(code) parse_expr_code(generate_parser(code).parse_expr())

TEST_F(ParserTest, SingleExpr) {
  // binary operator
  {
    char code[] = R"(2 + 3)";
    EXPECT_EQ(PARSE_CODE(code), "2 3 + ");
  }
  // unary operator
  {
    char code[] = R"(-7)";
    EXPECT_EQ(PARSE_CODE(code), "7 - ");
  }
  // function call
  {
    char code[] = R"(a(3, 5 ** 2))";
    EXPECT_EQ(PARSE_CODE(code), "a(3 , 5 2 ** , ) ");
  }
  // variable
  {
    char code[] = R"(abc)";
    EXPECT_EQ(PARSE_CODE(code), "abc ");
  }
  {
    char code[] = R"((a) c)";
    EXPECT_EQ(PARSE_CODE(code), "a ");
  }
  {
    char code[] = R"(orIgIn + T ** Rot + Roi)";
    EXPECT_EQ(PARSE_CODE(code), "origin t rot ** + Roi + ");
  }
  // tuple
  {
    char code[] = R"((1, 2, 3))";
    EXPECT_EQ(PARSE_CODE(code), "(1 , 2 , 3 , ) ");
  }
  {
    char code[] = R"((1, (3), 2 + 7, a*3, (2 + a(3, (4, 5))), (1, 2, 3 + 5)))";
    EXPECT_EQ(PARSE_CODE(code), "(1 , 3 , 2 7 + , a 3 * , 2 a(3 , (4 , 5 , ) , ) + , (1 , 2 , 3 5 + , ) , ) ");
  }
  // string
  {
    char code[] = R"("abc""bcd""ef")";
    EXPECT_EQ(PARSE_CODE(code), "abcbcdef ");
  }
  {
    char code[] = R"("Ab""cd)";
    EXPECT_EQ(PARSE_CODE(code), "Ab ");
  }
  {
    char code[] = R"("Ab\n""\\")";
    EXPECT_EQ(PARSE_CODE(code), "Ab\n\\ ");
  }
  {
    char code[] = R"("Ab\q")";
    EXPECT_EQ(PARSE_CODE(code), "Abq ");
    EXPECT_EQ(consumer.get_data(0)._result_diag_message, R"(unknown escape sequence '\q')");
  }
}

TEST_F(ParserTest, Precedence) {
  // only binary
  {
    char code[] = R"(2 + 3)";
    EXPECT_EQ(PARSE_CODE(code), "2 3 + ");
  }
  {
    char code[] = R"(1 + 2 * a - 5)";
    EXPECT_EQ(PARSE_CODE(code), "1 2 a * + 5 - ");
  }
  {
    char code[] = R"(1 ** 2 + 3 * 4 - 8)";
    EXPECT_EQ(PARSE_CODE(code), "1 2 ** 3 4 * + 8 - ");
  }
  {
    char code[] = R"(1 * 2 + 3 * 4 - 5 * 6 + 7 ** 8)";
    EXPECT_EQ(PARSE_CODE(code), "1 2 * 3 4 * + 5 6 * - 7 8 ** + ");
  }
  {
    char code[] = R"(1 ** (2 * ("abc""bcd\e" + 4)) - a(2 + 3) ** (5 ** 2))";
    EXPECT_EQ(PARSE_CODE(code), "1 2 abcbcde 4 + * ** a(2 3 + , ) 5 2 ** ** - ");
  }
  // with unary
  {
    char code[] = R"(1++++1)";  // parsed as 1 + (+ (+ (+ 1) ) )
    EXPECT_EQ(PARSE_CODE(code), "1 1 + + + + ");
  }
  {
    char code[] = R"(+++1+-2)";  // parsed as +(+(+1)) + (-2)
    EXPECT_EQ(PARSE_CODE(code), "1 + + + 2 - + ");
  }
  {
    char code[] = R"(++-a ** -b)";    // parsed as +(+(-(a ** (-b))))
    EXPECT_EQ(PARSE_CODE(code), "a b - ** - + + ");
  }
  {
    char code[] = R"(+a**b**c**d)";   // parsed as +(a**(b**(c**d)))
    EXPECT_EQ(PARSE_CODE(code), "a b c d ** ** ** + ");
  }
  {
    char code[] = R"(+a**(b**c)**d)";   // parsed as +(a**((b**c)**d)))
    EXPECT_EQ(PARSE_CODE(code), "a b c ** d ** ** + ");
  }
  {
    char code[] = R"(+a**-+b**c*d)";  // parsed as (+(a ** (-(+(b**c))) )) * d
    EXPECT_EQ(PARSE_CODE(code), "a b c ** + - ** + d * ");
  }
  // more complex
  {
    char code[] = R"((1, -2 ** +a(-3- -4)) ** f(a(3), (1+"ab\n", q)) * 2 ** -(3, 7)*2**4)";
    EXPECT_EQ(PARSE_CODE(code),
              "(1 , 2 a(3 - 4 - - , ) + ** - , ) f(a(3 , ) , (1 ab\n + , q , ) , ) ** 2 (3 , 7 , ) - ** * 2 4 ** * ");
  }
}

TEST_F(ParserTest, Diag) {
  {
    char code[] = R"((2 + 3)";
    EXPECT_EQ(PARSE_CODE(code), "error ast");
    EXPECT_EQ(consumer.get_data_size(), 2);
    EXPECT_EQ(consumer.get_data(0)._result_diag_message, "expected ')'");
    EXPECT_EQ(consumer.get_data(0).column_start_idx, 6);
    EXPECT_EQ(consumer.get_data(0).column_end_idx, 7);
    EXPECT_EQ(consumer.get_data(0).level, diag_data::ERROR);
    EXPECT_EQ(consumer.get_data(1)._result_diag_message, "to match this '('");
    EXPECT_EQ(consumer.get_data(1).column_start_idx, 0);
    EXPECT_EQ(consumer.get_data(1).column_end_idx, 1);
    EXPECT_EQ(consumer.get_data(1).level, diag_data::NOTE);
  }
  {
    char code[500];
    std::fill(code, code + 499, '9');
    code[499] = '\0';
    EXPECT_EQ(PARSE_CODE(code), "error ast");
    EXPECT_EQ(consumer.get_data_size(), 1);
    EXPECT_EQ(consumer.get_data(0)._result_diag_message,
              "constant literal is too large to be represented in a double type");
    EXPECT_EQ(consumer.get_data(0).column_start_idx, 0);
  }
  {
    char code[] = R"(*3)";
    EXPECT_EQ(PARSE_CODE(code), "error ast");
    EXPECT_EQ(consumer.get_data_size(), 1);
    EXPECT_EQ(consumer.get_data(0)._result_diag_message,
              "'*' cannot be a unary operator");
    EXPECT_EQ(consumer.get_data(0).column_start_idx, 0);
  }
  {
    char code[] = R"(3***1)";
    EXPECT_EQ(PARSE_CODE(code), "error ast");
    EXPECT_EQ(consumer.get_data_size(), 1);
    EXPECT_EQ(consumer.get_data(0)._result_diag_message,
              "'*' cannot be a unary operator");
    EXPECT_EQ(consumer.get_data(0).column_start_idx, 3);
  }
  {
    char code[] = R"(2 + a(3) **)";
    EXPECT_EQ(PARSE_CODE(code), "error ast");
    EXPECT_EQ(consumer.get_data_size(), 1);
    EXPECT_EQ(consumer.get_data(0)._result_diag_message,
              "expected expression");
    EXPECT_EQ(consumer.get_data(0).column_start_idx, 11);
  }
  {
    char code[] = R"(2 +* 3 +)";
    EXPECT_EQ(PARSE_CODE(code), "error ast");
    ASSERT_EQ(consumer.get_data_size(), 2);
    EXPECT_EQ(consumer.get_data(0)._result_diag_message,
              "'*' cannot be a unary operator");
    EXPECT_EQ(consumer.get_data(0).column_start_idx, 3);
    EXPECT_EQ(consumer.get_data(1)._result_diag_message,
              "expected expression");
    EXPECT_EQ(consumer.get_data(1).column_start_idx, 8);
  }
}

INTERPRETER_NAMESPACE_END
