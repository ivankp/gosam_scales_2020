// Written by Ivan Pogrebnyak

#ifndef IVANP_ERROR_HH
#define IVANP_ERROR_HH

#include <stdexcept>
#include "ivanp/string.hh"

namespace ivanp {

struct error: std::runtime_error {
  using std::runtime_error::runtime_error;
  template <typename... T>
  [[ gnu::always_inline ]]
  error(T&&... x): std::runtime_error(cat(std::forward<T>(x)...)) { };
  [[ gnu::always_inline ]]
  error(const char* str): std::runtime_error(str) { };
};

}

inline std::ostream& operator<<(std::ostream& s, const std::runtime_error& e) {
  return s << e.what();
}

#define IVANP_ERROR_STR1(x) #x
#define IVANP_ERROR_STR(x) IVANP_ERROR_STR1(x)

#define IVANP_ERROR_PREF __FILE__ ":" IVANP_ERROR_STR(__LINE__) ": "

#define THROW(...) throw ivanp::error(IVANP_ERROR_PREF, __VA_ARGS__);

#endif
