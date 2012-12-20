sources = plugin.cpp

all: plugin.so serializer.tsk

plugin.so: ${sources}
	clang++ plugin.cpp -fno-rtti ${CXXFLAGS} -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -shared -fPIC -o plugin.so -L/usr/local/lib -Wl,-rpath,/usr/local/lib -lclang -lLLVMSupport -std=c++11

serializer.tsk: serializer_example.cpp serializer.h json_serializer.h
	clang++ serializer_example.cpp -std=c++11 -o serializer.tsk

clean:
	rm plugin.so
	rm serializer.tsk

.PHONY: clean

