/**
 * This file defines the @code{diag_consumer} interface.
 *
 * @code{diag_consumer} is an abstract class with a virtual interface used to
 * accept a @code{diag_data} object. The implementation of this interface
 * should correctly use the various information stored in the object.
 *
 * @author 19030500131 zy
 */
#ifndef DRAWING_LANG_INTERPRETER_DIAGCONSUMER_H
#define DRAWING_LANG_INTERPRETER_DIAGCONSUMER_H

#include <Utils/def.h>

INTERPRETER_NAMESPACE_BEGIN

struct diag_data;

class diag_consumer {
public:
  virtual ~diag_consumer() = default;
  virtual void report(const diag_data* data) = 0;
};

class ignore_diag_consumer final : public diag_consumer {
public:
  void report(const diag_data*) override { /* ignore the data */ }
};

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_DIAGCONSUMER_H
