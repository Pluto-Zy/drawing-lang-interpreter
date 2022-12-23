#ifndef DRAWING_LANG_INTERPRETER_PARSERTEST_H
#define DRAWING_LANG_INTERPRETER_PARSERTEST_H

#include <MockTools.h>

INTERPRETER_NAMESPACE_BEGIN

class ParserTest : public ::testing::Test {
protected:
  diag_engine engine;
  test_diag_consumer consumer;
  std::unique_ptr<test_file_manager> manager;
  std::unique_ptr<lexer> l;

  void SetUp() override {
    engine.set_consumer(&consumer);
  }

  template<std::size_t N>
  parser generate_parser(const char(&str)[N]) {
    consumer.clear();
    manager = std::make_unique<test_file_manager>(str);
    engine.set_file(manager.get());
    l = std::make_unique<lexer>(manager.get(), engine);
    return parser(*l);
  }
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_PARSERTEST_H
