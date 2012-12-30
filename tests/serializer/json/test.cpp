#include <gtest/gtest.h>

#include <serializer/json.h>
#include <serializer/json/impl.h>

#include "resources.h"

class JsonTestFixture : public testing::Test {

};

bool equivalent(const JsonValue&, const JsonValue&);
bool equivalent(const JsonObject&, const JsonObject&);
bool equivalent(const JsonArray&, const JsonArray&);
bool equivalent(const JsonString&, const JsonString&);
bool equivalent(const JsonNumber&, const JsonNumber&);
bool equivalent(const JsonBool&, const JsonBool&);

bool equivalent(const JsonString& v1, const JsonString& v2) {
  return v1 == v2;
}

bool equivalent(const JsonNumber& v1, const JsonNumber& v2) {
  return v1 == v2;
}

bool equivalent(const JsonBool& v1, const JsonBool& v2) {
  return v1 == v2;
}

bool equivalent(const JsonArray& v1, const JsonArray& v2) {
  if (v1.size() != v2.size())
    return false;

  for (std::size_t i = 0; i < v1.size(); ++i) {
    if (!equivalent(v1[i], v2[i]))
      return false;
  }

  return true;
}

bool equivalent(const JsonObject& v1, const JsonObject& v2) {
  if (v1.size() != v2.size())
    return false;

  for (auto& p1 : v1) {
    bool key_match = false;
    for (auto& p2 : v2) {
      if (p1.first == p2.first) {
        key_match = true;
        if (!equivalent(p1.second, p2.second)) 
          return false;
        break;
      }
    }
    if (!key_match) 
      return false;
  }

  return true;
}

bool equivalent(const JsonValue& v1, const JsonValue& v2) {
  if (v1.type != v2.type)
    return false;

  switch (v1.type) {
    case JsonValue::OBJECT:
      return equivalent(static_cast<const JsonObject&>(v1), static_cast<const JsonObject&>(v2));
    case JsonValue::ARRAY:
      return equivalent(static_cast<const JsonArray&>(v1), static_cast<const JsonArray&>(v2));
    case JsonValue::STRING:
      return equivalent(static_cast<const JsonString&>(v1), static_cast<const JsonString&>(v2));
    case JsonValue::NUMBER:
      return equivalent(static_cast<const JsonNumber&>(v1), static_cast<const JsonNumber&>(v2));
    case JsonValue::BOOLEAN:
      return equivalent(static_cast<const JsonBool&>(v1), static_cast<const JsonBool&>(v2));
    default: {
      return true;
    }
  }
}

TEST_F(JsonTestFixture, t1) {
  JsonInStream ssi(text);
  JsonValue fill;
  format(ssi, fill);

/*
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
}
*/

  JsonValue v = 
    JsonObject{
      {"glossary", JsonObject{
        {"title", "example glossary"},
        {"GlossDiv", JsonObject{
          {"title", "S"},
          {"GlossList", JsonObject{
            {"GlossEntry", JsonObject{
              {"ID", "SGML"},
              {"SortAs", "SGML"},
              {"GlossTerm", "Standard Generalized Markup Language"},
              {"Acronym", "SGML"},
              {"Abbrev", "ISO 8879:1986"},
              {"GlossSee", "markup"},
              {"GlossDef", JsonObject {
                {"para", "A meta-markup language, used to create markup languages such as DocBook."},
                {"GlossSeeAlso", {"GML", "XML"}}
              }}
            }
          }}
        }} 
      }}
    }};
  
  ASSERT_TRUE(equivalent(fill, v));

  static_cast<JsonObject&>(v)["glossary"] = 1;
  ASSERT_FALSE(equivalent(fill, v));

  /*
  JsonOutStream ss;
  format(ss, fill);
  std::cout << std::endl;
  */
}
