// Written by Ivan Pogrebnyak

#ifndef IVANP_ENUMERATE_HH
#define IVANP_ENUMERATE_HH

#include <tuple>

namespace ivanp {
namespace detail { namespace enumerate {

template <typename Iter>
struct iterator {
  size_t i;
  Iter iter;
  bool operator!=(const iterator& o) const { return iter != o.iter; }
  iterator& operator++() { ++i; ++iter; return *this; }
  auto operator*() const { return std::forward_as_tuple(i,*iter); }
};

template <typename Iter>
iterator(size_t, Iter) -> iterator<Iter>;

template <typename T>
struct wrapper {
  T&& xs;
  auto begin() { return iterator{ 0, std::begin(std::forward<T>(xs)) }; }
  auto end() { return iterator{ 0, std::end(std::forward<T>(xs)) }; }
};

}}

template <typename T>
constexpr auto enumerate(T&& xs) {
  return detail::enumerate::wrapper<T>{ xs };
}

}

#endif
