
# traditional variables
CFLAGS:=-Wall -Wextra -g
CPPFLAGS:=-I include/
LDFLAGS:=

# object file targets
SRCS:=$(wildcard src/*/*.c)
SRCS+=$(filter-out %/main.c, $(wildcard src/*.c))

# object files containing a main() function
MSRCS:=$(wildcard src/main.c)
MSRCS+=$(wildcard test/*.c)

# compilation targets
OBJS:=$(patsubst %.c,%.o,$(SRCS))
MOBJS:=$(patsubst %.c,%.o,$(MSRCS))
TARGETS:=$(patsubst %.o,%,$(MOBJS))

# dependency files
DEPS:=$(patsubst %.o,%.d,$(OBJS) $(MOBJS))

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

%.unf.dot : %.ll_net
	@echo " UNF $<"
	@src/main $<

%.ll_net : %.xml
	@echo " P2P $<"
	@tools/pnml2pep.pl < $< > $@

%.r : %.dot
	@echo " RS  $<"
	@tools/rs.pl $< > $@

%-pr.ll_net : %.ll_net
	@echo " PR  $<"
	@tools/prenc.pl < $< > $@

