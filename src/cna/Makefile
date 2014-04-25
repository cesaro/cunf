
CC:=g++
CXX:=g++
LEX:=flex
YACC:=bison

CXXFLAGS:=-Wall -g -std=c++11 -DCONFIG_DEBUG
#CFLAGS:=-Wall -g -std=c99
CFLAGS:=$(CXXFLAGS)

.PHONY: all
all: test
	./test

test : test.o spec_lexer.o spec_parser.o spec.o
	$(CXX) $(CXXFLAGS) -o $@ $^

spec_lexer.o : spec_lexer.cc spec_parser.o
spec_parser.o : spec_parser.c

.PHONY: clean
clean:
	rm -rf *.o spec_lexer.cc spec_parser.h spec_parser.c test


.l.cc:
	flex -o $@ $^

.y.c:
	bison -o $@ $^
