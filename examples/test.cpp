double test(double x, double y) {
  auto z = x * x + y * y;
  return z;
}

template <typename type>
struct point {
  type x;
  type y;
  type z;
};

typedef point<double> pointd;

template <typename type>
struct vector {
  point<type> s;
  point<type> e;
};

typedef vector<double> vectord;

struct inherited : public vectord { };

int main(int argc, char* argv[]) {
  vector<int> test_vec;
}
