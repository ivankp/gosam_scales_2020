// Written by Ivan Pogrebnyak

#ifndef IVANP_TIMED_COUNTER_HH
#define IVANP_TIMED_COUNTER_HH

#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

#if __has_include(<unistd.h>)
#include <unistd.h>
#endif

namespace ivanp {

class comma_numpunct: public std::numpunct<char> {
protected:
  virtual char do_thousands_sep() const { return '\''; }
  virtual std::string do_grouping() const { return "\03"; }
};

template <typename CntType = unsigned long,
          typename Compare = std::less<CntType>>
class timed_counter {
  static_assert( std::is_integral<CntType>::value,
    "Cannot instantiate timed_counter of non-integral type");
public:
  using value_type   = CntType;
  using compare_type = Compare;
  using clock_type   = std::chrono::system_clock;
  using time_type    = std::chrono::time_point<clock_type>;
  using sec_type     = std::chrono::duration<double>;
  using ms_type      = std::chrono::duration<double,std::milli>;

private:
  value_type cnt, cnt_start, cnt_end;
  time_type start = clock_type::now(), last = start;
  compare_type cmp;

  static const std::locale cnt_locale;

public:
  void print() {
    using std::cout;
    using std::setw;
    using std::setfill;
    const std::ios::fmtflags f = cout.flags();
    auto prec = cout.precision();
    int nb = 30;

    const auto dt = sec_type((last = clock_type::now()) - start).count();
    const int hours   = dt/3600;
    const int minutes = (dt-hours*3600)/60;
    const int seconds = (dt-hours*3600-minutes*60);

    std::stringstream cnt_ss;
    cnt_ss.imbue(cnt_locale);
    cnt_ss << setw(14) << cnt;
    cout << cnt_ss.rdbuf() << " | ";
    if (cnt_end) {
      nb += 12;
      cout.precision(2);
      cout << std::fixed << setw(6) << (
        cnt==cnt_start ? 0. : 100.*float(cnt-cnt_start)/float(cnt_end-cnt_start)
      ) << "% | ";
    }
    if (hours) {
      nb += 8;
      cout << setw(5) << hours << ':'
      << setfill('0') << setw(2) << minutes << ':'
      << setw(2) << seconds << setfill(' ');
    } else if (minutes) {
      nb += 2;
      cout << setw(2) << minutes << ':'
      << setfill('0') << setw(2) << seconds << setfill(' ');
    } else if (seconds) {
      cout << setw(2) << seconds << 's';
    } else {
      cout << int(ms_type(clock_type::now() - start).count()) << "ms";
    }

#if __has_include(<unistd.h>)
    if (isatty(1)) {
      cout.flush();
      for (int i=nb; i; --i) cout << '\b';
    } else cout << std::endl;
#else
    cout.flush();
    for (int i=nb; i; --i) cout << '\b';
#endif
    cout.flags(f);
    cout.precision(prec);
  }
  void print_check() {
    if ( sec_type(clock_type::now()-last).count() > 1 ) print();
  }

  timed_counter(): cnt(0), cnt_start(0), cnt_end(0) { }
  timed_counter(value_type i, value_type n): cnt(i), cnt_start(i), cnt_end(n)
  { print(); }
  timed_counter(value_type n): cnt(0), cnt_start(0), cnt_end(n)
  { print(); }
  ~timed_counter() { print(); std::cout << std::endl; }

  void reset(value_type i, value_type n) {
    cnt = i;
    cnt_start = i;
    cnt_end = n;
    start = clock_type::now();
    last = start;
  }
  void reset(value_type n) { reset(0,n); }

  bool ok() const noexcept { return cmp(cnt,cnt_end); }
  bool operator!() const noexcept { return !ok(); }

  // prefix
  value_type operator++() { print_check(); return ++cnt; }
  value_type operator--() { print_check(); return --cnt; }

  // postfix
  value_type operator++(int) { print_check(); return cnt++; }
  value_type operator--(int) { print_check(); return cnt--; }

  template <typename T>
  value_type operator+= (T i) { print_check(); return cnt += i; }
  template <typename T>
  value_type operator-= (T i) { print_check(); return cnt -= i; }

  template <typename T>
  bool operator== (T i) const noexcept { return cnt == i; }
  template <typename T>
  bool operator!= (T i) const noexcept { return cnt != i; }
  template <typename T>
  bool operator<  (T i) const noexcept { return cnt <  i; }
  template <typename T>
  bool operator<= (T i) const noexcept { return cnt <= i; }
  template <typename T>
  bool operator>  (T i) const noexcept { return cnt >  i; }
  template <typename T>
  bool operator>= (T i) const noexcept { return cnt >= i; }

  // cast to integral type
  template <typename T> operator T () {
    static_assert( std::is_integral<T>::value,
      "Cannot cast timed_counter to a non-integral type" );
    return cnt;
  }

  value_type operator*() const { return cnt; }

  template <typename T>
  friend auto operator+(const timed_counter& c, T n) -> decltype(c.cnt+n) {
    return c.cnt + n;
  }

  friend std::ostream&
  operator<<(std::ostream& os, const timed_counter& tc)
  noexcept(noexcept(os << tc.cnt)) {
    return (os << tc.cnt);
  }
};

template <typename T, typename L>
const std::locale timed_counter<T,L>::cnt_locale(
  std::locale(), new comma_numpunct() );

} // end namespace

#endif
