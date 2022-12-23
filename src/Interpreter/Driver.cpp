#include <Diagnostic/DiagEngine.h>
#include <Diagnostic/DiagConsumer.h>
#include <Utils/FileManager.h>
#include <Sema/Sema.h>
#include <Interpret/InternalSupport/InternalImpl.h>
#include <Interpret/Interpreter.h>
#include <Lex/Lexer.h>
#include <Parse/Parser.h>

using namespace drawing;

int main(int argc, char* argv[]) {
  diag_engine diag;
  cmd_diag_consumer consumer;
  diag.set_consumer(&consumer);
  if (argc == 1) {
    diag.create_diag(drawing::err_no_input_file) << diag_build_finish;
    return 0;
  }
  file_manager manager;
  auto file_open_result = manager.from_file(argv[1]);
  if (file_open_result) {
    diag.create_diag(drawing::err_open_file) << argv[1] << diag_build_finish;
    return 0;
  }
  diag.set_file(&manager);
  symbol_table table;
  internal_impl internal;
  internal.export_all_symbols(table);
  sema action(diag, table);
  lexer l(&manager, diag);
  parser p(l);
  auto ast = p.parse_program();
  interpreter runner(action, internal);
  runner.run_stmts(std::move(ast));
  return 0;
}
