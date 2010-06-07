
# traditional variables
R:=$(shell pwd)
CFLAGS:=-Wall -Wextra -g
CPPFLAGS:=-I include/
LDFLAGS:=

# object file targets
SRCS:=$(filter-out %/main.c, $(wildcard $R/src/*.c))

OBJS:=$(patsubst %.c,%.o,$(SRCS))

# object files containing a main() function
MSRCS:=$(wildcard $R/src/main.c)

# tests
MSRCS+=$(wildcard $R/test/*.c)

MOBJS:=$(patsubst %.c,%.o,$(MSRCS))

# dependency files
DEPS:=$(patsubst %.o,%.d,$(OBJS)) $(patsubst %.o,%.d,$(MOBJS))

# compilation targets
TARGETS:=$(patsubst %.o,%,$(MOBJS))

# define the toolchain
CROSS:=

AS:=$(CROSS)as
LD:=$(CROSS)ld
CC:=$(CROSS)gcc
CXX:=$(CROSS)g++
CPP:=$(CROSS)cpp
AR:=$(CROSS)ar
NM:=$(CROSS)nm
OBJCPY:=$(CROSS)objcopy
OBJDUMP:=$(CROSS)objdump
STRIP:=$(CROSS)strip

%.d : %.c
	@echo " DEP $<"
	@set -e; $(CC) -MM -MT $*.o $(CPPFLAGS) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@;

.c.o:
	@echo " CC  $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.out : %.cml
	@echo " CNF $<"
	@./tools/cml2/cmlcompile.py -o $@ $<

%.pdf : %.dot
	@echo " DOT $<"
	@dot -T pdf < $< > $@

%.dot : %.ll_net
	@echo " N2D $<"
	@test/net2dot $< > $@

%.unf.dot : %
	@echo " UNF $<"
	@src/main $< > $@

%.ll_net : %.xml
	@echo " P2P $<"
	@tools/pnml2pep.pl < $< > $@

%.dot.r : %.dot
	@echo " RST $<"
	@tools/rs.pl $< > $@

