#include "json_serializer.h"

#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <set>

struct test {
  std::string _key;
  std::string _value;

  typedef std::string key_type;
  typedef std::string mapped_type;

  const std::string& key() const { return _key; }
  const std::string& value() const { return _value; }
};

template <>
struct format_override<test, JsonOutStream> {
	template <typename Stream>
  static void format(Stream& out, const test& obj) {
    ::format(out, obj._key);
    out << ":";
    ::format(out, obj._value);
  }
};

template <>
struct format_override<test, JsonInStream> {
	template <typename Stream>
  static void format(Stream& out, test& obj) {
    ::format(out, obj._key);
    out >> ":";
    ::format(out, obj._value);
  }
};

template <typename Stream>
struct format_override<test, Stream> {
  static void format(Stream& out, const test& obj) {
    ::format(out, obj._key);
    out << "->";
    ::format(out, obj._value);
  }
};

struct recursive {
  std::vector<int> vals;
  std::vector<recursive*> children;

  recursive() { }
  recursive(const std::initializer_list<int>& init) : vals(init) { }
};

template <>
struct format_override<recursive, JsonOutStream> {
	template <typename Stream>
  static void format(Stream& out, const recursive& obj) {
  	out << "{";
  	if (obj.vals.size()) {
    	out << "\"data\":";
    	::format(out, obj.vals);
    }
    if (obj.vals.size() && obj.children.size())
    	out << ",";
  	if (obj.children.size()) {
    	out << "\"children\":";
      ::format(out, obj.children);
    }
    out << "}";
  }
};

template <>
struct format_override<recursive, JsonInStream> {
	template <typename Stream>
  static void format(Stream& in, recursive& obj) {
    in >> "{";
    in >> "\"data\":";
    ::format(in, obj.vals);
    if (in)
    	in >> ",";
    in.good();

    in >> "\"children\":";
    ::format(in, obj.children);
    in.good();
    in >> "}";
  }
};

template <>
struct format_override<recursive*, JsonOutStream> {
	template <typename Stream>
  static void format(Stream& out, const recursive* obj) {
  	::format(out, *obj);
  }
};

template <>
struct format_override<recursive*, JsonInStream> {
	template <typename Stream>
  static void format(Stream& in, recursive*& obj) {
  	obj = new recursive();
  	::format(in, *obj);
  }
};

struct JsonValue;
typedef std::string JsonString;
typedef std::unordered_map<JsonString, JsonValue> JsonObject;
typedef std::vector<JsonValue> JsonArray;
typedef double JsonNumber;
typedef bool JsonBool;
struct JsonNull { };

template <>
struct format_override<JsonNull, JsonOutStream> {
  template <typename Stream>
  static void format(Stream& out, const JsonNull& obj) {
  	out << "null";
  }
};

template <>
struct format_override<JsonNull, JsonInStream> {
  template <typename Stream>
  static void format(Stream& in, JsonNull& obj) {
  	in.good();
  	in >> "null";
  }
};

template <>
struct format_override<JsonBool, JsonOutStream> {
	template <typename Stream>
	static void format(Stream& out, const JsonBool& obj) {
		if (obj)
			out << "true";
		else
			out << "false";
	}
};

template <>
struct format_override<JsonBool, JsonInStream> {
  template <typename Stream>
  static void format(Stream& in, JsonBool& obj) {
  	in.good();
  	in >> "true";
  	if (in) {
  		obj = true;
  	  return;
  	}

    in.good();
  	in >> "false";
  	if (in) {
  		obj = false;
  		return;
  	}
  }
};

struct JsonValue {
  union {
  	JsonObject* object;
  	JsonArray* array;
  	JsonString* string;
  	JsonNumber* number;
  	JsonBool* boolean;
  } ptr;
  
  enum TYPE : unsigned char {
    OBJECT,
    ARRAY,
    STRING,
    NUMBER,
    BOOLEAN,
    NULL_
  } type;

  JsonValue()
    : type(NULL_) { }

  JsonValue(int i) {
  	*this = new JsonNumber(i);
  }

  JsonValue(double d) {
  	*this = new JsonNumber(d);
  }

  JsonValue(std::string str) {
  	*this = new JsonString(std::move(str));
  }

  JsonValue(const char* str) {
  	*this = new JsonString(str);
  }

  JsonValue(bool b) {
  	*this = new JsonBool(b);
  }

  JsonValue(JsonArray arr) {
  	*this = new JsonArray(std::move(arr));
  }

  JsonValue(std::initializer_list<JsonValue> arr) {
  	*this = new JsonArray(arr);
  }

  JsonValue(std::initializer_list<std::pair<const JsonString, JsonValue>> pairs) {
  	*this = new JsonObject(pairs);
  }

  JsonValue& operator = (JsonObject* _ptr) {
    ptr.object = _ptr;
    type = OBJECT;
    return *this;
  }

  JsonValue& operator = (JsonObject&& _val) {
  	ptr.object = new JsonObject(std::move(_val));
  	type = OBJECT;
  	return *this;
  }

  JsonValue& operator = (JsonArray* _ptr) {
  	ptr.array = _ptr;
  	type = ARRAY;
  	return *this;
  }

  JsonValue& operator = (JsonArray&& _val) {
  	ptr.array = new JsonArray(_val);
  	type = ARRAY;
  	return *this;
  }

  JsonValue& operator = (JsonString* _ptr) {
  	ptr.string = _ptr;
  	type = STRING;
  	return *this;
  }

  JsonValue& operator = (const JsonString& val) {
  	ptr.string = new JsonString(val);
  	type = STRING;
  	return *this;
  }
 
  JsonValue& operator = (JsonNumber* _ptr) {
  	ptr.number = _ptr;
  	type = NUMBER;
  	return *this;
  }

  JsonValue& operator = (double _val) {
    ptr.number = new JsonNumber(_val);
    type = NUMBER;
    return *this;
  }

  JsonValue& operator = (JsonBool* _ptr) {
  	ptr.boolean = _ptr;
  	type = BOOLEAN;
  	return *this;
  }

  JsonValue& operator = (bool _val) {
  	ptr.boolean = new bool(_val);
  	type = BOOLEAN;
  	return *this;
  }

  JsonValue& operator = (const JsonNull& _val) {
  	type = NULL_;
  	return *this;
  }
};

typedef typename JsonObject::value_type JsonPair;

template <>
struct format_override<JsonValue, JsonOutStream> {
	template <typename Stream>
  static void format(Stream& out, const JsonValue& value) {
  	switch(value.type) {
  		case JsonValue::OBJECT:
  		  ::format(out, *value.ptr.object);
  		  break;
  		case JsonValue::ARRAY:
  		  ::format(out, *value.ptr.array);
  		  break;
  		case JsonValue::STRING:
  		  ::format(out, *value.ptr.string);
  		  break;
  		case JsonValue::NUMBER:
  		  ::format(out, *value.ptr.number);
  		  break;
  		case JsonValue::BOOLEAN:
  		  ::format(out, *value.ptr.boolean);
  		  break;
  		case JsonValue::NULL_:
  		  ::format(out, JsonNull());
  		  break;
  	}
  }
};

template <>
struct format_override<JsonValue, JsonInStream> {
	template <typename Stream>
  static void format(Stream& in, JsonValue& value) {
  	in.good();
  	JsonString s;
  	::format(in, s);
  	if (in) {
  		value = s;
  		return;
  	}

    in.good();
  	JsonNumber n;
  	::format(in, n);
  	if (in) {
  		value = n;
  		return;
  	}


    in.good();
    JsonBool b;
    ::format(in, b);
    if (in) {
    	value = b;
    	return;
    }

    in.good();
    JsonNull nu;
    ::format(in, nu);
    if (in) {
    	value = nu;
    	return;
    }

    in.good();
    JsonObject ob;
    ::format(in, ob);
    if (in) {
    	value = std::move(ob);
    	return;
    }

    in.good();
    JsonArray ar;
    ::format(in, ar);
    if (in) {
    	value = std::move(ar);
    	return;
    }
  }
};

void test1() {
  constexpr auto text = R"({"First":["hello","world","goodbye"],"Second":["Meh"]})";
  JsonInStream ssi(text);
  std::unordered_map<std::string, std::set<std::string>> fill;
  format(ssi, fill);

  JsonOutStream ss;
  format(ss, fill);
  std::cout << std::endl;
}

void test2() {
	constexpr auto text = R"({"First":{"a":1,"b":2,"c":3,"d":4},"Second":{"e":5,"f":6,"g":7,"h":8}})";
  JsonInStream ssi(text);
  std::unordered_map<std::string, std::map<std::string, int>> fill;
  format(ssi, fill);

  JsonOutStream ss;
  format(ss, fill);
  std::cout << std::endl;
}

void test3() {
	constexpr auto text = R"({"Hello": "Goodbye", 	"Hello2": "Goodbye2"})";
  JsonInStream ssi(text);
  std::vector<test> fill;
  format(ssi, fill);

  JsonOutStream ss;
  format(ss, fill);
  std::cout << std::endl;
}

void test4() {
	constexpr auto text = R"(
		{
		  "data": [1, 2, 3], 
		  "children": [
		  	{
		  		"data": [4, 5, 6]
		  	},
		  	{
		  		"data": [7, 8, 9]
		  	},
		  	{
		  		"children": [
		  			{
		  				"data": [10, 11]
		  			}
		  		]
		  	}
		  ]
    }
  )";
	JsonInStream ssi(text);
	recursive fill;
	format(ssi, fill);

  JsonOutStream ss;
  format(ss, fill);
	std::cout << std::endl;
}

void test5() {
	constexpr auto text = R"([1, 2, 3, 4, 5])";
	JsonInStream ssi(text);
	std::vector<int> fill;
	format(ssi, fill);

	JsonOutStream ss;
	format(ss, fill);
	std::cout << std::endl;
}

void test6() {
  JsonObject obj;
  obj["Hello"] = JsonString("World");
  obj["Goodbye"] = false;
  JsonArray arr = {1, "hi", "bye", true, false, {"hello", "world", 1, 2, 3}};
  JsonObject obj2 = {{"Hello", 1}, {"Goodbye", 2}, {"Test", {JsonPair{"?", 2}}}};
  obj["Children"] = std::move(arr);
  obj["Sub-obj"] = std::move(obj2);

  JsonOutStream ss;
  format(ss, obj);
}

void test7() {
  constexpr auto text = R"(
{
    "glossary": {
        "title": "example glossary",
		"GlossDiv": {
            "title": "S",
			"GlossList": {
                "GlossEntry": {
                    "ID": "SGML",
					"SortAs": "SGML",
					"GlossTerm": "Standard Generalized Markup Language",
					"Acronym": "SGML",
					"Abbrev": "ISO 8879:1986",
					"GlossDef": {
                        "para": "A meta-markup language, used to create markup languages such as DocBook.",
						"GlossSeeAlso": ["GML", "XML"]
                    },
					"GlossSee": "markup"
                }
            }
        }
    }
})";
	JsonInStream ssi(text);
	JsonObject fill;
	format(ssi, fill);

	JsonOutStream ss;
	format(ss, fill);
	std::cout << std::endl;
}

int main(int argc, char* argv[]) {
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  test7();
}