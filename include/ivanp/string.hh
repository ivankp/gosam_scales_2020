#ifndef IVANP_STRING_HH
#define IVANP_STRING_HH

#include <sstream>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ivanp {

template <typename Stream, typename... T>
[[ gnu::always_inline ]]
inline Stream&& stream(Stream&& s, T&&... x) {
  (s << ... << std::forward<T>(x));
  return s;
}

template <typename... T>
[[ gnu::always_inline ]]
inline auto cat(T&&... x) -> std::enable_if_t<
  !(... && std::is_convertible_v<T&&,std::string_view>),
  std::string
> {
  return static_cast<std::stringstream&&>(
    (std::stringstream() << ... << std::forward<T>(x))
  ).str();
}
inline std::string cat() noexcept { return { }; }
inline std::string cat(std::string x) noexcept { return x; }
inline std::string cat(const char* x) noexcept { return x; }

inline std::string cat(std::initializer_list<std::string_view> xs) noexcept {
  size_t len = 0;
  for (const auto& x : xs) len += x.size();
  std::string s;
  s.reserve(len);
  for (const auto& x : xs) s.append(x);
  return s;
}

template <typename... T>
[[ gnu::always_inline ]]
inline auto cat(T&&... x) noexcept -> std::enable_if_t<
  (... && std::is_convertible_v<T&&,std::string_view>),
  std::string
> { return cat({std::forward<T>(x)...}); }

// ------------------------------------------------------------------

inline const char* cstr(const char* str) noexcept { return str; }
inline char* cstr(char* str) noexcept { return str; }
inline const char* cstr(const std::string& str) noexcept { return str.data(); }
inline char* cstr(std::string& str) noexcept { return str.data(); }

struct chars_less {
  using is_transparent = void;
  bool operator()(const char* a, const char* b) const noexcept {
    return strcmp(a,b) < 0;
  }
  template <typename T>
  bool operator()(const T& a, const char* b) const noexcept {
    return strncmp(a.data(),b,a.size()) < 0;
  }
  template <typename T>
  bool operator()(const char* a, const T& b) const noexcept {
    return strncmp(a,b.data(),b.size()) < 0;
  }
};

inline bool starts_with(const char* str, const char* prefix) noexcept {
  for (;; ++str, ++prefix) {
    if (!*prefix) break;
    if (*str != *prefix) return false;
  }
  return true;
}

inline bool ends_with(const char* str, const char* suffix) noexcept {
  const auto n1 = strlen(str);
  const auto n2 = strlen(suffix);
  if (n1<n2) return false;
  return starts_with(str+(n1-n2),suffix);
}

} // end namespace ivanp

#endif
