CFLAGS=-O2 -pedantic -Wall -Werror
CXXFLAGS=-std=c++20

all: plpqsort

plpqsort: plpqsort.cc
	$(CXX) $(CFLAGS) $(CXXFLAGS) $^ -o $@

run: plpqsort
	./$^ || true

lint: plpqsort.cc
	cpplint.py $^
