#include "gl3.h"

#include <string>
#include <utility>

int f(int x) { return 0; }

class Inner {
  double z;
};

class HelloWorld {
  int x;
  float y;
  Inner i;

  void empty_return() { }
};

int y;

namespace {
  int x1;
  int x2;
}

namespace test {
  int y;
}

namespace test2 {
  namespace test3 {
    int z;

    int doit() { return 0; };
    int dontdoit();
  }
}

enum hello {
  there,
  world,
  loser
};

enum class hello2 : unsigned int {
  there,
  world,
  loser
};

void print() { }

template <typename Head, typename... Args>
void print(Head&& head, Args&&... args) { 
  print(head);
  print(std::forward<Args>(args)...);
}

constexpr int doubledown(int x) { return 1; }

int main(int argc, char* argv[]) {
  std::string meh;
  print(1, 2, 3, 4, "Five", "Six", "Seven", "Eight");
}
