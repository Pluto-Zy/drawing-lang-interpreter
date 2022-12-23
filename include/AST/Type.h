/**
 * This file implements a simple type system.
 *
 * @author 19030500131
 */
#ifndef DRAWING_LANG_INTERPRETER_TYPE_H
#define DRAWING_LANG_INTERPRETER_TYPE_H

#include <Utils/def.h>
#include <memory>
#include <cassert>
#include <vector>
#include <any>
#include <string>

INTERPRETER_NAMESPACE_BEGIN

/**
 * Defines the correspondence between the types in the language
 * and the actual types.
 */
using TUPLE_T = std::vector<std::any>;
#define BASIC_TYPE(NAME, TYPE, SPELLING) using NAME##_T = TYPE;
#include <Interpret/TypeDef.h>

/**
 * This class represents a type in the drawing language. There are
 * some basic types (such as Integer, String and so on) and a nested
 * type (tuple) in the language. If it is a tuple, then it will have
 * a sub type (and @code{has_sub_type()} returns @code{true}). Nested
 * types can be subtypes of nested types, which actually forms a
 * chain structure.
 *
 * The memory of the subtype is managed by the outer nested type.
 * Copying a @code{type} object will copy all its subtype objects
 * (ie deep copy).
 */
class type {
public:
  enum type_kind : unsigned char {
#define BASIC_TYPE(NAME, T, S) NAME,
#include <Interpret/TypeDef.h>
    TUPLE,
  };
  explicit type(type_kind kind) : _kind(kind), _sub_type(nullptr) { }
  type(type_kind kind, std::unique_ptr<type> sub) : _kind(kind), _sub_type(std::move(sub)) { }
  type(const type& other) : _kind(other._kind), _sub_type(nullptr) {
    if (other._sub_type) {
      _sub_type = std::make_unique<type>(*other._sub_type);
    }
  }
  type(type&&) = default;
  type& operator=(const type& other) {
    if (this != &other) {
      _kind = other._kind;
      if (other._sub_type) {
        _sub_type = std::make_unique<type>(*other._sub_type);
      }
    }
    return *this;
  }
  type& operator=(type&&) = default;
  [[nodiscard]] type_kind get_kind() const { return _kind; };
  [[nodiscard]] bool has_sub_type() const { return _sub_type != nullptr; }
  [[nodiscard]] const type& get_sub_type() const { assert(has_sub_type()); return *_sub_type; }
  [[nodiscard]] type& get_sub_type() { assert(has_sub_type()); return *_sub_type; }
  void add_sub_type(std::unique_ptr<type> sub) {
    assert(!has_sub_type() && sub);
    _sub_type = std::move(sub);
  }
  [[nodiscard]] std::string get_spelling() const;

  [[nodiscard]] bool is(type_kind kind) const { return _kind == kind; }
  [[nodiscard]] bool is_not(type_kind kind) const { return _kind != kind; }
private:
  type_kind _kind;
  std::unique_ptr<type> _sub_type;
};

/**
 * Two types are equal if and only if they are the same kind(type_kind)
 * and the subtypes are also equal.
 */
inline bool operator==(const type& lhs, const type& rhs) {
  if (lhs.get_kind() != rhs.get_kind())
    return false;
  if (!lhs.has_sub_type() && !rhs.has_sub_type())
    return true;
  if (lhs.has_sub_type() && rhs.has_sub_type())
    return lhs.get_sub_type() == rhs.get_sub_type();
  return false;
}

inline bool operator!=(const type& lhs, const type& rhs) {
  return !(lhs == rhs);
}

/**
 * Returns the spelling of the type used for diag.
 */
inline std::string type::get_spelling() const {
  switch (_kind) {
    case TUPLE:
      assert(_sub_type);
      return "TUPLE<" + _sub_type->get_spelling() + ">";
#define BASIC_TYPE(NAME, TYPE, SPELLING) case NAME: return SPELLING;
#include <Interpret/TypeDef.h>
  }
  assert(false);
  return "";
}

/**
 * Below are some tool implementations for converting values.
 *
 * In order to support the different types in the language,
 * the values of these different types are all stored uniformly
 * using @code{std::any} objects.
 *
 * For basic types, it's easy to pack and unpack the value.
 * Take Integer as an example,
 *
 *    pack   - std::make_any<INTEGER_T>(value)
 *    unpack - std::any_cast<INTEGER_T>(value)
 *
 * For nested types, it's more complicated to pack and unpack
 * the value. Values of nested types usually need to be packed
 * or unpacked layer by layer. For example, if we need to pack
 * a value of type @code{Tuple<Tuple<Integer>>}, we will start
 * from some @code{INTEGER_T} objects and convert them to some
 * @code{std::any} objects. Then save all the objects to a
 * @code{std::vector<std::any>}. Then we should pack the vector
 * to @code{std::any} object and also save some objects of this
 * type to a @code{std::vector<std::any>}, and then convert it
 * to a whole @code{std::any} object.
 *
 * The process of unpacking is the opposite. We should first
 * @code{std::any_cast} the @code{std::any} object to
 * @code{std::vector<std::any>}. Then convert the elements of the
 * vector (of type @code{std::any}) to @code{std::vector<std::any>}
 * again. Lastly we convert all the elements of the vectors to
 * @code{INTEGER_T} and use them. If we directly convert the
 * initial @code{std::any} object to @code{std::vector<std::vector<INTEGER_T>>},
 * we will get an @code{std::bad_any_cast} exception.
 *
 * The function @code{pack_value} and @code{unpack_value} will do
 * all the jobs automatically. We can call
 * @code{unpack_value<std::vector<std::vector<INTEGER_T>>>(value)}
 * to unpack a @code{std::any} object and the @code{unpack_value}
 * method will unpack it layer by layer.
 *
 * @code{get_type} function will return the corresponding @code{type}
 * object according to its template argument. For example,
 * @code{get_type<INTEGER_T>} will return a @code{type} represents
 * INTEGER.
 */
namespace {
template<class Ty>
struct get_type_impl;

#define BASIC_TYPE(NAME, TYPE, SPELLING) \
template<> struct get_type_impl<TYPE> {  \
  static type get_type() {               \
    return type(type::NAME);             \
  }                                      \
};
#include <Interpret/TypeDef.h>

// for tuple
template<class Ty>
struct get_type_impl<std::vector<Ty>> {
  static type get_type() {
    return type(type::TUPLE, std::make_unique<type>(get_type_impl<Ty>::get_type()));
  }
};

template<class ArgTy>
struct _prepare_arg_impl {
  static ArgTy get_arg(std::any arg) {
    return std::any_cast<ArgTy>(arg);
  }
};
template<class ElemTy>
struct _prepare_arg_impl<std::vector<ElemTy>> {
  static std::vector<ElemTy> get_arg(std::any arg) {
    auto vec = std::any_cast<std::vector<std::any>>(std::move(arg));
    std::vector<ElemTy> result;
    result.reserve(vec.size());
    for (auto& elem : vec) {
      result.emplace_back(_prepare_arg_impl<ElemTy>::get_arg(std::move(elem)));
    }
    return result;
  }
};
template<class Ty>
struct _make_ret_value_impl {
  static_assert(!std::is_reference_v<Ty>);
  static std::any make(Ty value) {
    return std::make_any<Ty>(std::move(value));
  }
};
template<class Ty>
struct _make_ret_value_impl<std::vector<Ty>> {
  static std::any make(std::vector<Ty> value) {
    std::vector<std::any> result;
    result.reserve(value.size());
    for (Ty& elem : value) {
      result.emplace_back(_make_ret_value_impl<Ty>::make(std::move(elem)));
    }
    return result;
  }
};
}

template<class Ty>
type get_type() {
  return get_type_impl<Ty>::get_type();
}

template<class... Ty>
std::vector<type> get_types() {
  std::vector<type> result;
  result.reserve(sizeof...(Ty));
  (result.emplace_back(get_type<Ty>()), ...);
  return result;
}

template<class Ty>
Ty unpack_value(std::any val) {
  return _prepare_arg_impl<Ty>::get_arg(val);
}

template<class Ty>
std::any pack_value(Ty val) {
  return _make_ret_value_impl<Ty>::make(val);
}

INTERPRETER_NAMESPACE_END

#endif //DRAWING_LANG_INTERPRETER_TYPE_H
