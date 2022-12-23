#include <MockTools.h>
#include <AST/StmtVisitor.h>
#include <Sema/Sema.h>
#include <sstream>
#include "ParserTest.h"

INTERPRETER_NAMESPACE_BEGIN

namespace {
class stmt_printer : public stmt_visitor<stmt_printer> {
  std::string result;
public:
  std::string take_result() && { return std::move(result); }

  void visit_assignment_stmt(assignment_stmt* s) {
    ASSERT_TRUE(s);
    ASSERT_EQ(s->get_assignment_lhs()->get_stmt_kind(), stmt::variable_expr_type);
    variable_expr* lhs = static_cast<variable_expr*>(s->get_assignment_lhs());
    result += "assign " + static_cast<std::string>(lhs->get_name());
    result += ' ' + std::to_string(s->get_is_loc());
    result += get_expr_range(s->get_assignment_rhs());
    result += '\n';
  }

  void visit_for_stmt(for_stmt* s) {
    ASSERT_TRUE(s);
    result += "for ";
    ASSERT_EQ(s->get_for_expr()->get_stmt_kind(), stmt::variable_expr_type);
    auto for_var = static_cast<variable_expr*>(s->get_for_expr());
    result += static_cast<std::string>(for_var->get_name());
    if (s->has_from()) {
      result += " from " + std::to_string(s->get_from_loc()) + get_expr_range(s->get_from_expr());
    }
    result += " to " + std::to_string(s->get_to_loc()) + get_expr_range(s->get_to_expr());
    if (s->has_step()) {
      result += " step " + std::to_string(s->get_step_loc()) + get_expr_range(s->get_step_expr());
    }
    result += '\n';
    for (auto iter = s->body_begin(); iter != s->body_end(); ++iter) {
      result += '\t';
      visit(iter->get());
    }
  }

  void visit_expr_stmt(expr_stmt* s) {
    ASSERT_TRUE(s);
    result += "expr" + get_expr_range(s->get_expr());
    result += '\n';
  }

  void visit_empty_stmt(empty_stmt* s) {
    ASSERT_TRUE(s);
    result += "empty\n";
  }
private:
  static std::string get_expr_range(expr* e) {
    return ' ' + std::to_string(e->get_start_loc()) + ' ' + std::to_string(e->get_end_loc());
  }
};

std::string get_stmt_str(std::vector<stmt_result_t> s) {
  stmt_printer printer;
  for (auto& elem : s)
    if (elem)
      printer.visit(elem.get());
  return std::move(printer).take_result();
}
}

#define PARSE_CODE(code) get_stmt_str(generate_parser(code).parse_program())

TEST_F(ParserTest, SingleStmt) {
  // empty statement
  {
    char code[] = ";";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "empty\n");
  }
  // assignment stmt
  {
    char code[] = "a is 123 + b;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "assign a 2 5 12\n");
  }
  {
    char code[] = "abc is f(123);";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "assign abc 4 7 13\n");
  }
  // expr stmt
  {
    char code[] = "1 + 2;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "expr 0 5\n");
  }
  {
    char code[] = "a;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "expr 0 1\n");
  }
  {
    char code[] = "(1, 2, (3, 4));";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "expr 0 14\n");
  }
  // for stmt
  {
    char code[] = "for abc from 0.5 to 1.3 step 2;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "for abc from 8 13 16 to 17 20 23 step 24 29 30\n\tempty\n");
  }
  {
    char code[] = "for abc to 1.3 step 2;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "for abc to 8 11 14 step 15 20 21\n\tempty\n");
  }
  {
    char code[] = "for abc from 0.5 to 1.3;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "for abc from 8 13 16 to 17 20 23\n\tempty\n");
  }
  {
    char code[] = "for abc from 0.5 to 1.3 a(3);";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "for abc from 8 13 16 to 17 20 23\n\texpr 24 28\n");
  }
  {
    char code[] = "for abc from 0.5 to 1.3 { origin is 234; origin + f(3); }";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "for abc from 8 13 16 to 17 20 23\n\tassign origin 33 36 39\n\texpr 41 54\n");
  }
  {
    char code[] = "for abc to 1.3 step a b;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "for abc to 8 11 14 step 15 20 21\n\texpr 22 23\n");
  }
}

TEST_F(ParserTest, MultiStmts) {
  {
    char code[] = "abc is a;abc + ab;abc(a);";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "assign abc 4 7 8\nexpr 9 17\nexpr 18 24\n");
  }
  {
    char code[] = "for t to 3 steq + origin;for color to 4 step 1 1;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "for t to 6 9 10\n\texpr 11 24\nfor color to 35 38 39 step 40 45 46\n\texpr 47 48\n");
  }
  {
    char code[] = "for t to 3 { steq + origin;for color to 4 step 1 {1; origin is roi(origin); }";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "for t to 6 9 10\n\texpr 13 26\n\tfor color to 37 40 41 step 42 47 48\n\texpr 50 51\n\tassign origin 60 63 74\n");
  }
}

TEST_F(ParserTest, StmtDiag) {
  {
    char code[] = "abc it 123;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "assign abc 4 7 10\n");
    EXPECT_EQ(consumer.get_data_size(), 1);
    EXPECT_EQ(consumer.get_data(0).fix.code_to_insert, "is");
  }
  {
    char code[] = "is 123;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "");
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  {
    char code[] = "for a to a { a is a";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "for a to 6 9 10\n\tassign a 15 18 19\n");
    EXPECT_EQ(consumer.get_data_size(), 3);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
    std::cout << consumer.get_data(1)._result_diag_message << std::endl;
    std::cout << consumer.get_data(2)._result_diag_message << std::endl;
  }
  {
    char code[] = "for a + 4 from 1 to a;";
    auto result = PARSE_CODE(code);
    EXPECT_EQ(result, "");
    EXPECT_EQ(consumer.get_data_size(), 1);
    std::cout << consumer.get_data(0)._result_diag_message << std::endl;
  }
  {
    char code[] = R"("abc""ac)";
    EXPECT_EQ(PARSE_CODE(code), "expr 0 5\n");
    ASSERT_EQ(consumer.get_data_size(), 3);
    EXPECT_EQ(consumer.get_data(0)._result_diag_message,
              "missing terminating '\"' character");
    EXPECT_EQ(consumer.get_data(0).column_start_idx, 5);
    EXPECT_EQ(consumer.get_data(1)._result_diag_message,
              "expected ';' after expression");
    EXPECT_EQ(consumer.get_data(1).column_start_idx, 5);
    EXPECT_EQ(consumer.get_data(2)._result_diag_message,
              "expected expression");
    EXPECT_EQ(consumer.get_data(2).column_start_idx, 5);
  }
}

INTERPRETER_NAMESPACE_END
