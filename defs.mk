
# Copyright (C) 2010, 2011  Cesar Rodriguez <cesar.rodriguez@lsv.ens-cachan.fr>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.

# traditional variables
CFLAGS:=-Wall -Wextra -O3
#CFLAGS:=-Wall -Wextra -O3 -pg
#CFLAGS:=-Wall -Wextra -pg
#CFLAGS:=-Wall -Wextra
#CFLAGS:=-Wall -Wextra -g
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

# testing
TEST_NETS:=$(shell tools/testnets.sh)
TEST_R:=$(patsubst %.ll_net,%.r,$(TEST_NETS))
TEST_UNF_R:=$(patsubst %.ll_net,%.unf.r,$(TEST_NETS))

# measuring times
#TIME_NETS:=$(patsubst %.xml,%.ll_net,$(wildcard test/nets/param/*.xml))
TIME_NETS+=$(wildcard test/nets/plain/small/*.ll_net)
TIME_NETS+=$(wildcard test/nets/cont/small/*.ll_net)
TIME_NETS+=$(wildcard test/nets/pr/small/*.ll_net)

TIME_NETS+=$(wildcard test/nets/plain/med/*.ll_net)
TIME_NETS+=$(wildcard test/nets/cont/med/*.ll_net)
TIME_NETS+=$(wildcard test/nets/pr/med/*.ll_net)

TIME_NETS+=$(wildcard test/nets/plain/large/*.ll_net)
TIME_NETS+=$(wildcard test/nets/cont/large/*.ll_net)
TIME_NETS+=$(wildcard test/nets/pr/large/*.ll_net)

# checking for deadlock
DEAD_NETS:=$(wildcard test/nets/tiny/dead/*.ll_net)
DEAD_NETS+=$(wildcard test/nets/plain/small/*.ll_net)
#DEAD_NETS+=$(wildcard test/nets/cont/small/*.ll_net)
#DEAD_NETS+=$(wildcard test/nets/pr/small/*.ll_net)

#DEAD_NETS+=$(wildcard test/nets/plain/med/*.ll_net)
#DEAD_NETS+=$(wildcard test/nets/cont/med/*.ll_net)
#DEAD_NETS+=$(wildcard test/nets/pr/med/*.ll_net)

#DEAD_NETS+=$(wildcard test/nets/plain/large/*.ll_net)
#DEAD_NETS+=$(wildcard test/nets/cont/large/*.ll_net)
#DEAD_NETS+=$(wildcard test/nets/pr/large/*.ll_net)

# define the toolchain
CROSS:=

LD:=$(CROSS)ld
CC:=$(CROSS)gcc
CXX:=$(CROSS)g++
CPP:=$(CROSS)cpp

%.d : %.c
	@echo "DEP $<"
	@set -e; $(CC) -MM -MT $*.o $(CPPFLAGS) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@;

.c.o:
	@echo "CC  $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.pdf : %.dot
	@echo "DOT $<"
	@dot -T pdf < $< > $@

%.jpg : %.dot
	@echo "DOT $<"
	@dot -T jpg < $< > $@

%.dot : %.ll_net
	@echo "N2D $<"
	@test/net2dot $< > $@

%.dot : %.cuf
	@echo "C2D $<"
	@test/cuf2dot $<

%.unf.cuf : %.ll_net
	@echo "UNF $<"
	@src/main $< | grep -C 17 cutoffs
	@#src/main $<

%.unf.mci : %.ll_net
	@echo "MLE $<"
	@mole $< -m $@

%.dead1 : %.unf.mci
	@mcsmodels $< | grep 'FALSE\|TRUE' | (read R; /bin/echo -e "$$R\t$@")

%.mcs : %.mci
	@mcsmodels $<

%.dead2 : %.unf.cnf
	@minisat $< | grep SATISFIABLE | (read R; /bin/echo -e "$$R\t$@")

%.sat : %.cnf
	minisat $<; true

%.cnf : %.bc
	time bc2cnf < $< > $@

%.bc : %.cuf
	time tools/unf2bc < $< > $@

%.time : %.ll_net
	@tools/time.sh src/main $< 2>&1

%.info : %.ll_net
	@test/info $< | tools/distrib.py

%.ll_net : %.xml
	@echo "P2P $<"
	@tools/pnml2pep.pl < $< > $@

%.r : %.dot
	@echo "RS  $<"
	@tools/rs.pl $< > $@
