#include "serializer.h"

#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <set>


std::unordered_map<std::string, std::set<std::string>> contents;

struct test {
  std::string _key;
  std::string _value;

  typedef std::string key_type;
  typedef std::string mapped_type;

  const std::string& key() const { return _key; }
  const std::string& value() const { return _value; }
};

template <>
struct format_override<test, JsonStream> {
  static void format(JsonStream& out, const test& obj) {
    ::format(out, obj._key);
    out << " : ";
    ::format(out, obj._value);
  }
};

template <typename Stream>
struct format_override<test, Stream> {
  static void format(Stream& out, const test& obj) {
    ::format(out, obj._key);
    out << " -> ";
    ::format(out, obj._value);
  }
};

struct recursive {
  std::vector<int> vals;
  std::vector<recursive*> children;

  recursive(const std::initializer_list<int>& init) : vals(init) { }
};

template <typename Stream>
struct format_override<recursive, Stream> {
  static void format(Stream& out, const recursive& obj) {
  	out << "{ serializer: ";
  	::format(out, obj.vals);
  	out << ", children: ";
    ::format(out, obj.children);
    out << " } ";
  }
};

template <typename Stream>
struct format_override<recursive*, Stream> {
  static void format(Stream& out, const recursive* obj) {
  	::format(out, *obj);
  }
};

std::vector<test> tests;

int main(int argc, char* argv[]) {
  auto& vec = contents["test"];
  vec.insert("one");
  vec.insert("two");
  vec.insert("three");

  auto& vec2 = contents["test2"];
  vec2.insert("four");
  vec2.insert("five");
  vec2.insert("six");

  JsonStream ss;

  tests.push_back({"First", "Up"});
  tests.push_back({"Hello", "World"});
  tests.push_back({"Goodbye", "World"});

  format(ss, contents);
  format(std::cout, contents);
  format(ss, tests);
  format(std::cout, tests);

  std::cout << std::endl;

  recursive s({1,2,3});
  s.children.push_back(new recursive({4,5,6}));
  format(ss, s);
}