#include <string>
#include <iostream>

template <typename>
struct Void {
  typedef void type;
};

#define has_member(name, sig) \
template <typename T, typename sfinae = void> \
struct has_##name : std::false_type { }; \
\
template <typename T> \
struct has_##name<T, typename Void<decltype(std::declval<T&>().sig)>::type> : std::true_type { }; \


#define has_typedef(name, td) \
template <typename T, typename sfinae = void> \
struct has_##name : std::false_type { typedef void type; }; \
\
template <typename T> \
struct has_##name<T, typename Void<typename T::td>::type> : std::true_type { typedef typename T::td type; }; \


#define has_static_member(name, stat) \
template <typename T, typename sfinae = void> \
struct has_##name : std::false_type { }; \
\
template <typename T> \
struct has_##name<T, typename Void<decltype(T::stat)>::type> : std::true_type { }; \

template <typename T, typename Stream = std::ostream>
struct formatter { };

template <typename T, typename Stream = std::ostream>
struct format_override { };

template <typename T, typename Stream = std::ostream, typename sfinae = void>
struct has_format_override : std::false_type { };

template <typename T, typename Stream>
struct has_format_override<T, Stream, typename Void<decltype(format_override<T, Stream>::format(std::declval<Stream&>(), std::declval<T&>()))>::type> : std::true_type { };

template <typename T>
struct string_type : std::false_type { };

template <>
struct string_type<std::string> : std::true_type { };

template <>
struct string_type<const char*> : std::true_type { };

has_member(begin, begin())
has_member(end, end())
has_member(first, first)
has_member(second, second)
has_member(key_field, key);
has_member(value_field, value);
has_static_member(format, format())
has_typedef(key_type, key_type)
has_typedef(mapped_type, mapped_type)
has_typedef(value_type, value_type)

template <typename T>
struct has_key {
  typedef typename has_key_type<T>::type key_type;
  typedef typename has_value_type<T>::type value_type;

  const static bool value = (has_key_type<T>::value && string_type<key_type>::value && has_mapped_type<T>::value)
                         || (has_value_type<T>::value && has_key<value_type>::value);
};

template <>
struct has_key<void> {
  typedef void key_type;
  typedef void value_type;

  const static bool value = false;
};

template <typename T>
struct has_pair_accessors {
  const static bool value = has_first<T>::value && has_second<T>::value;
};

template <typename T>
struct has_keyval_accessors {
  const static bool value = has_key_field<T>::value && has_value_field<T>::value;
};

template <typename T>
struct has_range {
  typedef typename has_value_type<T>::type value_type;
  const static bool value = has_begin<T>::value && has_end<T>::value;
};

template <typename T>
auto getKey(const T& t) -> decltype(t.first) {
  return t.first;
}

template <typename T>
auto getKey(const T& t) -> decltype(t.key) {
  return t.key;
}

template <typename T>
auto getKey(const T& t) -> decltype(t.key()) {
  return t.key();
}

template <typename T>
auto getValue(const T& t) -> decltype(t.second) {
  return t.second;
}

template <typename T>
auto getValue(const T& t) -> decltype(t.value) {
  return t.value;
}

template <typename T>
auto getValue(const T& t) -> decltype(t.value()) {
  return t.value();
}

struct JsonStream : public std::ostream {
  template <typename T>
  JsonStream& operator << (const T& obj) {
    std::cout << obj;
    return *this;
  }
};

template <typename T, typename Stream>
void format(Stream& out, const T& obj) {
  formatter<T, Stream>::format(out, obj);
}

template <typename U>
struct formatter<U, std::ostream> {
  typedef std::ostream Stream;

  template <typename T>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<has_range<T>::value && not has_key<T>::value>::type {
    std::string sep = "";
    out << "<";
    for (const auto& itr : t) {
      out << sep;
      format(out, itr);
      sep = " : ";
    }
    out << ">";
  }

  template <typename T>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<has_key<T>::value && !has_format_override<typename has_key<T>::value_type, Stream>::value>::type {
    std::string sep = "";
    out << "(";
    for (const auto& itr : t) {
      out << sep;
      format(out, getKey(itr));
      out << " -> ";
      format(out, getValue(itr));
      sep = ", ";
    }
    out << ")";
  }

  template <typename T>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<has_key<T>::value && has_format_override<typename has_key<T>::value_type, Stream>::value>::type {
    std::string sep = "";
    out << "(";
    for (const auto& itr : t) {
      out << sep;
      format(out, itr);
      sep = ", ";
    }
    out << ")";
  }

  template <typename T>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<!(has_range<T>::value || has_key<T>::value)>::type {
    out << t;
  }

  template <typename T>
  static auto format(Stream& out, const T& t) -> typename std::enable_if<has_format_override<T, Stream>::value>::type {
    format_override<T, Stream>::format(out, t);
  }

  template <typename T>
  static auto format(Stream& out, const T& t) -> typename std::enable_if<!has_format_override<T, Stream>::value>::type {
    format_impl(out, t);
  }
};

template <typename U>
struct formatter<U, JsonStream> {
  typedef JsonStream Stream;

  template <typename T>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<has_range<T>::value && not has_key<T>::value>::type {
    std::string sep = "";
    out << "[";
    for (const auto& itr : t) {
      out << sep;
      format(out, itr);
      sep = ", ";
    }
    out << "]";
  }

  template <typename T>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<has_key<T>::value && !has_format_override<typename has_key<T>::value_type, Stream>::value>::type {
    std::string sep = "";
    out << "{";
    for (const auto& itr : t) {
      out << sep;
      format(out, getKey(itr));
      out << " : ";
      format(out, getValue(itr));
      sep = ", ";
    }
    out << "}";
  }

  template <typename T>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<has_key<T>::value && has_format_override<typename has_key<T>::value_type, Stream>::value>::type {
    std::string sep = "";
    out << "{";
    for (const auto& itr : t) {
      out << sep;
      format(out, itr);
      sep = ", ";
    }
    out << "}";
  }

  template <typename T>
  static auto format_impl(Stream& out, const T& t) -> typename std::enable_if<!(has_range<T>::value || has_key<T>::value)>::type {
    out << t;
  }

  template <typename T>
  static auto format(Stream& out, const T& t) -> typename std::enable_if<has_format_override<T, Stream>::value>::type {
    format_override<T, Stream>::format(out, t);
  }

  template <typename T>
  static auto format(Stream& out, const T& t) -> typename std::enable_if<!has_format_override<T, Stream>::value>::type {
    format_impl(out, t);
  }
};

template <>
struct format_override<std::string, JsonStream> {
  static void format(JsonStream& out, const std::string& obj) {
    out << "\"" << obj << "\"";
  }
};

template <typename T1, typename T2>
struct format_override<std::pair<T1, T2>, JsonStream> {
  static void format(JsonStream& out, const std::pair<T1, T2>& obj) {
    ::format(out, obj.first);
    out << " : ";
    ::format(out, obj.second);
  }
};

template <typename Stream>
struct format_override<std::string, Stream> {
  static void format(Stream& out, const std::string& obj) {
    out << obj;
  }
};
