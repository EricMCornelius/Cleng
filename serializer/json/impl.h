#pragma once

#include <serializer/json.h>

#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <memory>

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
  union value_type {
    std::shared_ptr<JsonNull> null;
    std::shared_ptr<JsonObject> object;
    std::shared_ptr<JsonArray> array;
    std::shared_ptr<JsonString> string;
    std::shared_ptr<JsonNumber> number;
    std::shared_ptr<JsonBool> boolean;

    value_type() : null(nullptr) { }
    ~value_type() { }
  } ptr;
  
  enum TYPE : unsigned char {
    OBJECT,
    ARRAY,
    STRING,
    NUMBER,
    BOOLEAN,
    NULL_
  } type;

  operator JsonObject& () {
    return *ptr.object;
  }

  operator const JsonObject& () const {
    return *ptr.object;
  }

  operator JsonArray& () {
    return *ptr.array;
  }

  operator const JsonArray& () const {
    return *ptr.array;
  }

  operator JsonString& () {
    return *ptr.string;
  }

  operator const JsonString& () const {
    return *ptr.string;
  }

  operator JsonNumber& () {
    return *ptr.number;
  }

  operator const JsonNumber& () const {
    return *ptr.number;
  }

  operator JsonBool& () {
    return *ptr.boolean;
  }

  operator const JsonBool& () const {
    return *ptr.boolean;
  }

  JsonValue()
    : type(NULL_) { }

  JsonValue(int i) {
    *this = static_cast<double>(i);
  }

  JsonValue(double d) {
    *this = d;
  }

  JsonValue(std::string&& str) {
    *this = str;
  }

  JsonValue(const char* str) {
    *this = str;
  }

  JsonValue(bool b) {
    *this = b;
  }

  JsonValue(JsonArray arr) {
    *this = arr;
  }

  JsonValue(JsonObject obj) {
    *this = obj;
  }

  JsonValue(std::initializer_list<JsonValue> arr) {
    *this = JsonArray(arr);
  }

  JsonValue(std::initializer_list<std::pair<const JsonString, JsonValue>> pairs) {
    *this = JsonObject(pairs);
  }

  JsonValue(const JsonValue& other) {
    *this = other;
  }

  JsonValue(JsonValue&& other) {
    *this = other; 
  }

  ~JsonValue() {
    cleanup();
  }

  void cleanup() {
    switch (type) {
      case OBJECT:
        ptr.object = nullptr;
        break;
      case ARRAY:
        ptr.array = nullptr;
        break;
      case STRING:
        ptr.string = nullptr;
        break;
      case NUMBER:
        ptr.number = nullptr;
        break;
      case BOOLEAN:
        ptr.boolean = nullptr;
        break;
      default: {

      }
    }
    type = NULL_;
  }

  JsonValue& operator = (JsonObject _val) {
    ptr.object = std::make_shared<JsonObject>(_val);
    type = OBJECT;
    return *this;
  }

  JsonValue& operator = (JsonArray _val) {
    ptr.array = std::make_shared<JsonArray>(_val);
    type = ARRAY;
    return *this;
  }

  JsonValue& operator = (JsonString _val) {
    ptr.string = std::make_shared<JsonString>(_val);
    type = STRING;
    return *this;
  }

  JsonValue& operator = (const char* _str) {
    ptr.string = std::make_shared<JsonString>(_str);
    type = STRING;
    return *this;
  }
 
  JsonValue& operator = (JsonNumber _val) {
    ptr.number = std::make_shared<JsonNumber>(_val);
    type = NUMBER;
    return *this;
  }

  JsonValue& operator = (int _val) {
    ptr.number = std::make_shared<JsonNumber>(_val);
    type = NUMBER;
    return *this;
  }

  JsonValue& operator = (JsonBool _val) {
    ptr.boolean = std::make_shared<JsonBool>(_val);
    type = BOOLEAN;
    return *this;
  }

  JsonValue& operator = (const JsonNull& _val) {
    type = NULL_;
    return *this;
  }

  JsonValue& operator = (const JsonValue& _val) {
    if (this == &_val)
      return *this;

    cleanup();
    type = _val.type;
    switch (_val.type) {
      case OBJECT:
        ptr.object = _val.ptr.object;
        break;
      case ARRAY:
        ptr.array = _val.ptr.array;
        break;
      case STRING:
        ptr.string = _val.ptr.string;
        break;
      case NUMBER:
        ptr.number = _val.ptr.number;
        break;
      case BOOLEAN:
        ptr.boolean = _val.ptr.boolean;
        break;
      default: {

      }
    }
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
