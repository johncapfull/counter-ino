CXX := clang++
CXX_FLAGS := -stdlib=libc++ -std=c++11 -DTEST
CPP_FILES := $(wildcard *.cpp)

test: $(CPP_FILES)
	$(CXX) -O2 -o test $(CXX_FLAGS) $^