sources = plugin.cpp

all: plugin.so serializer.tsk test.tsk

plugin.so: ${sources}
	clang++ plugin.cpp -I. -fno-rtti ${CXXFLAGS} -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -shared -fPIC -o plugin.so -L/usr/local/lib -Wl,-rpath,/usr/local/lib -lclang -lLLVMSupport -std=c++11

serializer.tsk: serializer/example.cpp serializer/core.h serializer/json.h
	clang++ -I. serializer/example.cpp -std=c++11 -O2 -o serializer.tsk

test.tsk: tests/serializer/json/test.cpp
	clang++ -I. tests/serializer/json/test.cpp -std=c++11 -O2 -lgtest_main -lgtest -o test.tsk

clean:
	rm plugin.so
	rm serializer.tsk

.PHONY: clean

