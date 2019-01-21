// Copyright 2017 Global Phasing Ltd.
//
// Utilities.

#ifndef GEMMI_UTIL_HPP_
#define GEMMI_UTIL_HPP_

#include <algorithm>  // for equal, find, remove_if
#include <cctype>     // for tolower
#include <iterator>   // for begin, end, make_move_iterator
#include <stdexcept>  // for runtime_error
#include <string>
#include <vector>

namespace gemmi {

//   #####   string helpers   #####

inline bool starts_with(const std::string& str, const std::string& prefix) {
  size_t sl = prefix.length();
  return str.length() >= sl && str.compare(0, sl, prefix) == 0;
}

inline bool ends_with(const std::string& str, const std::string& suffix) {
  size_t sl = suffix.length();
  return str.length() >= sl && str.compare(str.length() - sl, sl, suffix) == 0;
}

// Case-insensitive comparisons. The second arg must be lowercase.

inline bool iequal(const std::string& str, const std::string& low) {
  return str.length() == low.length() &&
         std::equal(std::begin(low), std::end(low), str.begin(),
                    [](char c1, char c2) { return c1 == std::tolower(c2); });
}

inline bool istarts_with(const std::string& str, const std::string& prefix) {
  return str.length() >= prefix.length() &&
         std::equal(std::begin(prefix), std::end(prefix), str.begin(),
                    [](char c1, char c2) { return c1 == std::tolower(c2); });
}
inline bool iends_with(const std::string& str, const std::string& suffix) {
  size_t sl = suffix.length();
  return str.length() >= sl &&
         std::equal(std::begin(suffix), std::end(suffix), str.end() - sl,
                    [](char c1, char c2) { return c1 == std::tolower(c2); });
}

inline bool giends_with(const std::string& str, const std::string& suffix) {
  return iends_with(str, suffix) || iends_with(str, suffix + ".gz");
}

inline std::string to_lower(std::string str) {
  for (char& c : str)
    if (c >= 'A' && c <= 'Z')
      c |= 0x20;
  return str;
}

inline std::string to_upper(std::string str) {
  for (char& c : str)
    if (c >= 'a' && c <= 'z')
      c &= ~0x20;
  return str;
}

inline std::string trim_str(const std::string& str) {
  const std::string ws = " \r\n\t";
  std::string::size_type first = str.find_first_not_of(ws);
  if (first == std::string::npos)
    return std::string{};
  std::string::size_type last = str.find_last_not_of(ws);
  return str.substr(first, last - first + 1);
}

inline std::string rtrim_str(const std::string& str) {
  std::string::size_type last = str.find_last_not_of(" \r\n\t");
  return str.substr(0, last == std::string::npos ? 0 : last + 1);
}

// end is after the last character of the string (typically \0)
inline const char* rtrim_cstr(const char* start, const char* end=nullptr) {
  if (!start)
    return nullptr;
  if (!end) {
    end = start;
    while (*end != '\0')
      ++end;
  }
  while (end > start && std::isspace(end[-1]))
    --end;
  return end;
}

namespace impl {
inline size_t length(char) { return 1; }
inline size_t length(const std::string& s) { return s.length(); }
}

// takes a single separator (usually char or string);
// may return empty fields
template<typename S>
void split_str_into(const std::string& str, S sep,
                    std::vector<std::string>& result) {
  std::size_t start = 0, end;
  while ((end = str.find(sep, start)) != std::string::npos) {
    result.emplace_back(str, start, end - start);
    start = end + impl::length(sep);
  }
  result.emplace_back(str, start);
}

template<typename S>
std::vector<std::string> split_str(const std::string& str, S sep) {
  std::vector<std::string> result;
  split_str_into(str, sep, result);
  return result;
}

// _multi variants takes multiple 1-char separators as a string;
// discards empty fields
inline void split_str_into_multi(const std::string& str, const char* seps,
                                 std::vector<std::string>& result) {
  std::size_t start = str.find_first_not_of(seps);
  while (start != std::string::npos) {
    std::size_t end = str.find_first_of(seps, start);
    result.emplace_back(str, start, end - start);
    start = str.find_first_not_of(seps, end);
  }
}

inline std::vector<std::string> split_str_multi(const std::string& str,
                                                const char* seps=" \t") {
  std::vector<std::string> result;
  split_str_into_multi(str, seps, result);
  return result;
}

template<typename T, typename S, typename F>
std::string join_str(const T& iterable, const S& sep, const F& getter) {
  std::string r;
  bool first = true;
  for (const auto& item : iterable) {
    if (!first)
      r += sep;
    r += getter(item);
    first = false;
  }
  return r;
}

template<typename T, typename S>
std::string join_str(const T& iterable, const S& sep) {
  return join_str(iterable, sep, [](const std::string& t) { return t; });
}

inline const char* skip_blank(const char* p) {
  if (p)
    while (*p == ' ' || *p == '\t')
      ++p;
  return p;
}


//   #####   vector helpers   #####

template <class T>
bool in_vector(const T& x, const std::vector<T>& v) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

template <class T>
void vector_move_extend(std::vector<T>& dst, std::vector<T>&& src) {
  if (dst.empty())
    dst = std::move(src);
  else
    dst.insert(dst.end(), std::make_move_iterator(src.begin()),
                          std::make_move_iterator(src.end()));
}

// wrapper around the erase-remove idiom
template <class T, typename F>
void vector_remove_if(std::vector<T>& v, F&& condition) {
  v.erase(std::remove_if(v.begin(), v.end(), condition), v.end());
}


//   #####   other helpers   #####

// simplified version of C++17 std::clamp
template<class T> const T& clamp_(const T& v, const T& lo, const T& hi)
{
  return v < lo ? lo : hi < v ? hi : v;
}

// Numeric ID used for case-insensitive comparison of 4 letters.
// s must have 4 chars or 3 chars + NUL, ' ' and NUL are equivalent in s.
inline int ialpha4_id(const char* s) {
  return (s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]) & ~0x20202020;
}

[[noreturn]]
inline void fail(const std::string& msg) { throw std::runtime_error(msg); }

// unreachable() is used to silence GCC -Wreturn-type and hint the compiler
[[noreturn]] inline void unreachable() {
#if defined(__GNUC__) || defined(__clang__)
  __builtin_unreachable();
#elif defined(_MSC_VER)
  __assume(0);
#endif
}

} // namespace gemmi
#endif
// vim:sw=2:ts=2:et
