#include <stdio.h>

struct test_struct;

typedef int TestType;

#define TEST_MACRO 1;

int test(const TestType* x, double y);

struct test_struct {
  const int x;
  struct test_embedded_struct {
    int x;
  } z;
};

struct {
	int x;
	int y;
} z;

struct test_second_struct {
  int* x;
  int z;
  int y;
  const struct test_struct* f;
};

int test(const TestType* x, double y) { 
  return 0;
}

int main(int argc, char* argv[]) {
  const int x = 0;
  double y;
  test(&x, y);
}

struct test_struct;
