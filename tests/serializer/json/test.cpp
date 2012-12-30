#include <gtest/gtest.h>

#include <serializer/json.h>
#include <serializer/json/impl.h>

#include "resources.h"

class JsonTestFixture : public testing::Test {

};

TEST_F(JsonTestFixture, t1) {
  JsonInStream ssi(text);
  JsonObject fill;
  format(ssi, fill);
  
  JsonOutStream ss;
  format(ss, fill);
  std::cout << std::endl;
}