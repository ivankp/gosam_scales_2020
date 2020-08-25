#ifndef IVANP_MATH_HH
#define IVANP_MATH_HH

namespace ivanp {

template <typename... T>
[[ gnu::always_inline ]]
constexpr auto sq(const T&... x) noexcept { return (... + (x*x)); }

}

#endif
