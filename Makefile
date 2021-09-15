all: test

test: test.cc circular_buffer.h
	g++ -I `pkg-config --cflags gtest` test.cc `pkg-config --libs gtest` -o test
