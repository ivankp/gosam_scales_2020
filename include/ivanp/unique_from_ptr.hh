// Written by Ivan Pogrebnyak

#ifndef IVANP_UNIQUE_FROM_PTR_HH
#define IVANP_UNIQUE_FROM_PTR_HH

#include <memory>
#include <type_traits>

template <typename T, typename U>
using also_const_noref_t = std::conditional_t< std::is_const_v<U>,
  const std::remove_reference_t<T>, std::remove_reference_t<T> >;

template <typename T, typename U>
auto unique_from_ptr(U* ptr) noexcept -> std::enable_if_t<
  !std::is_same_v<T,U>,
  std::unique_ptr<also_const_noref_t<T,U>>
> {
  using type = also_const_noref_t<T,U>;
  return std::unique_ptr<type>( static_cast<type*>(ptr) );
}
template <typename T>
auto unique_from_ptr(T* ptr) noexcept {
  return std::unique_ptr<T>( ptr );
}

#endif
