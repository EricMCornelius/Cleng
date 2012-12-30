#pragma once

#include <serializer/core.h>
#include <iterator>

struct JsonOutStream {
  std::ostringstream buffer;
};

template <typename T>
JsonOutStream& operator << (JsonOutStream& out, const T& obj) {
  std::cout << obj;
  return out;
}

struct JsonInStream {
  std::istringstream buffer;
  operator bool() { return buffer; }
  void good() { buffer.clear(); }

  JsonInStream(const std::string& contents) : buffer(contents) { }
};

template <typename T>
JsonInStream& operator >> (JsonInStream& in, T& obj) {
  in.buffer >> obj;
  return in;
}

JsonInStream& operator >> (JsonInStream& in, std::string& obj) {
  char c;
  bool escape = false;
  while(in.buffer >> std::noskipws >> c) {
    if (!escape && c == '"') {
      in.buffer.putback(c);
      break;
    }
    obj += c;
    escape = (c == '\\' && !escape);
  }
  return in;
}

JsonInStream& operator >> (JsonInStream& in, const char* str) {
  std::size_t len = std::strlen(str);
  std::size_t count = 0;
  char c;

  auto pos = in.buffer.tellg();

  while(count < len && in.buffer >> c) {
    if (std::isspace(c))
      continue;

    if (c != str[count])
      break;
    ++count;
  }
  if (count != len) {
    in.buffer.seekg(pos);
    in.buffer.setstate(std::ios_base::badbit);
  }

  return in;
}


template <typename U>
struct formatter<U, JsonOutStream> {
  template <typename T, typename Stream>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<has_range<T>::value && not has_key<T>::value>::type {
    const char* sep = "";
    out << "[";
    for (const auto& itr : t) {
      out << sep;
      format(out, itr);
      sep = ",";
    }
    out << "]";
  }

  template <typename T, typename Stream>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<has_key<T>::value && !has_format_override<typename has_key<T>::value_type, Stream>::value>::type {
    const char* sep = "";
    out << "{";
    for (const auto& itr : t) {
      out << sep;
      format(out, getKey(itr));
      out << ":";
      format(out, getValue(itr));
      sep = ",";
    }
    out << "}";
  }

  template <typename T, typename Stream>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<has_key<T>::value && has_format_override<typename has_key<T>::value_type, Stream>::value>::type {
    const char* sep = "";
    out << "{";
    for (const auto& itr : t) {
      out << sep;
      format(out, itr);
      sep = ",";
    }
    out << "}";
  }

  template <typename T, typename Stream>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<!(has_range<T>::value || has_key<T>::value)>::type {
    out << t;
  }

  template <typename T, typename Stream>
  static auto format(Stream& out, const T& t) -> typename std::enable_if<has_format_override<T, Stream>::value>::type {
    format_override<T, Stream>::format(out, t);
  }

  template <typename T, typename Stream>
  static auto format(Stream& out, const T& t) -> typename std::enable_if<!has_format_override<T, Stream>::value>::type {
    format_impl(out, t);
  }
};

template <typename U>
struct formatter<U, JsonInStream> {
  template <typename T, typename Stream>
  static auto format_impl(Stream& out, T& t) -> typename std::enable_if<has_range<T>::value && not has_key<T>::value, bool>::type {
    typedef typename has_range<T>::value_type value_type;
    auto itr = std::inserter(t, t.begin());

    if (!(out >> "["))
      return false;

    while(true) {
      value_type obj;
      if (!format(out, obj))
        break;
      *itr = obj;
      if (!(out >> ","))
        break;
    }

    out.good();
    if (!(out >> "]"))
      return false;

    return out;
  }

  template <typename T, typename Stream>
  static auto format_impl(Stream& out, T& t) -> typename std::enable_if<has_key<T>::value && !has_format_override<typename has_key<T>::value_type, Stream>::value, bool>::type {
    typedef typename has_key<T>::key_type key_type;
    typedef typename has_key<T>::mapped_type mapped_type;
    typedef typename has_key<T>::value_type value_type;
    auto itr = std::inserter(t, t.begin());

    if (!(out >> "{"))
      return false;

    while(true) {
      key_type key;
      mapped_type value;
      if (!format(out, key))
        break;
      if (!(out >> ":"))
        break;
      if (!format(out, value))
        break;
      *itr = value_type(key, value);
      
      if (!(out >> ","))
        break;
    }

    out.good();
    if (!(out >> "}"))
      return false;

    return out;
  }

  template <typename T, typename Stream>
  static auto format_impl(Stream& out, T& t) -> typename std::enable_if<has_key<T>::value && has_format_override<typename has_key<T>::value_type, Stream>::value, bool>::type {
    typedef typename has_key<T>::value_type value_type;
    auto itr = std::inserter(t, t.begin());

    if (!(out >> "{"))
      return false;

    while(true) {
      value_type obj;
      if (!format(out, obj))
        break;
      *itr = obj;

      if (!(out >> ","))
        break;
    }

    out.good();
    if (!(out >> "}"))
      return false;

    return out;
  }

  template <typename T, typename Stream>
  static auto format_impl(Stream& out, T& t) -> typename std::enable_if<!(has_range<T>::value || has_key<T>::value), bool>::type {
    out >> t;
    return out;
  }

  template <typename T, typename Stream>
  static auto format(Stream& out, T& t) -> typename std::enable_if<has_format_override<T, Stream>::value, bool>::type {
    format_override<T, Stream>::format(out, t);
    return out;
  }

  template <typename T, typename Stream>
  static auto format(Stream& out, T& t) -> typename std::enable_if<!has_format_override<T, Stream>::value, bool>::type {
    format_impl(out, t);
    return out;
  }
};

template <>
struct format_override<std::string, JsonOutStream> {
  template <typename Stream>
  static void format(Stream& out, const std::string& obj) {
    out << "\"" << obj << "\"";
  }
};

template <>
struct format_override<std::string, JsonInStream> {
  template <typename Stream>
  static void format(Stream& out, std::string& obj) {
    out >> "\"" >> obj >> "\"";
  }
};
