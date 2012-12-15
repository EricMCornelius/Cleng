sources = plugin.cpp

plugin.so: ${sources}
	clang++ plugin.cpp -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -shared -fPIC -o plugin.so -L/usr/lib64/llvm -lclang -std=c++11 -Wno-dangling-field

all: plugin.so

clean:
	rm plugin.so

.PHONY: all clean

