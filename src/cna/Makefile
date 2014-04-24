
CXX:=g++
CC:=$(CXX)
LEX:=flex

CDEBUG = -g -Wall
CFLAGS = -Wall -g -std=c99
CXXFLAGS = -Wall -g -std=c++11

.PHONY: all
all: test
	./test < input

test : test.o spec_lexer.o
	$(CXX) $(CXXFLAGS) -o $@ $^

spec_lexer.o : spec_lexer.cc

.PHONY: clean
clean:
	rm -rf *.o spec_lexer.cc test

.l.cc:
	$(LEX) -o $@ $^
