
# Copyright (C) 2010--2014  Cesar Rodriguez <cesar.rodriguez@cs.ox.ac.uk>
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

.DEFAULT_GOAL := all

include config.mk

# traditional variables
ifdef CONFIG_DEBUG
CFLAGS = -Wall -Wextra -std=c11 -g # -pg -emit-llvm
CXXFLAGS = -Wall -Wextra -std=c++11 -g # -pg -emit-llvm
endif
ifdef CONFIG_RELEASE
CFLAGS = -Wall -Wextra -std=c11 -O3 # -g -pg
CXXFLAGS = -Wall -Wextra -std=c++11 -O3 # -g -pg
endif
CPPFLAGS:=-I src/ -D_POSIX_C_SOURCE=200809L -D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS -D NDEBUG
#LDFLAGS :=
#LDLIBS :=

# source code
SRCS:=$(wildcard src/*.c src/*.cc src/*/*.c src/*/*.cc src/*/*/*.c src/*/*/*.cc)
SRCS:=$(filter-out %/cunf.cc, $(SRCS))
SRCS:=$(filter-out %/pep2dot.c, $(SRCS))
SRCS:=$(filter-out %/pep2pt.c, $(SRCS))
SRCS:=$(SRCS) src/cna/spec_lexer.cc src/cna/spec_parser.cc

# do not remove files generated by lex or bison once you generate them
.SECONDARY: src/cna/spec_lexer.cc src/cna/spec_parser.cc src/cna/spec_parser.h

# source code containing a main() function
MSRCS:=$(wildcard src/cunf/cunf.cc src/pep2dot.c src/pep2pt.c)

# compilation targets
OBJS:=$(SRCS:.cc=.o)
OBJS:=$(OBJS:.c=.o)
MOBJS:=$(MSRCS:.cc=.o)
MOBJS:=$(MOBJS:.c=.o)
TARGETS:=$(MOBJS:.o=)

# list of nets for several tasks
TEST_NETS:=$(shell scripts/nets.sh test)
TIME_NETS:=$(shell scripts/nets.sh time)
ALL_NETS:=$(shell scripts/nets.sh no-huge)
MCI_NETS:=$(shell scripts/nets.sh mci)
DEAD_NETS:=$(MCI_NETS)
CNMC_NETS:=$(shell scripts/nets.sh all | grep -v huge)

# define the toolchain
VERS:=
LD:=llvm-ld$(VERS)
CC:=clang$(VERS)
CXX:=clang++$(VERS)
CPP:=cpp$(VERS)
LEX:=flex
YACC:=bison

%.d : %.c
	@echo "DEP $<"
	@set -e; $(CC) -MM -MT $*.o $(CFLAGS) $(CPPFLAGS) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@;

%.d : %.cc
	@echo "DEP $<"
	@set -e; $(CXX) -MM -MT $*.o $(CXXFLAGS) $(CPPFLAGS) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@;

%.cc : %.l
	@echo "LEX $<"
	@$(LEX) -o $@ $^

%.cc %.h : %.y
	@echo "YAC $<"
	@$(YACC) -o $(^:.y=.cc) $^

# cancelling gnu make builtin rules for lex/yacc to c
# http://ftp.gnu.org/old-gnu/Manuals/make-3.79.1/html_chapter/make_10.html#SEC104
%.c : %.l
%.c : %.y

COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
COMPILE.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
LINK.c = $(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
LINK.cc = $(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o : %.c
	@echo "CC  $<"
	@$(COMPILE.c)

%.o : %.cc
	@echo "CXX $<"
	@$(COMPILE.cc)

%.i : %.c
	@echo "CC  $<"
	@$(COMPILE.c) -E

%.i : %.cc
	@echo "CXX $<"
	@$(COMPILE.cc) -E

%.pdf : %.dot
	@echo "DOT $<"
	@dot -T pdf < $< > $@

%.jpg : %.dot
	@echo "DOT $<"
	@dot -T jpg < $< > $@

%.dot : %.ll_net
	@echo "P2D $<"
	@src/pep2dot $< > $@
	@#scripts/pep2dot.py < $< > $@

%.ll_net : %.cuf
	@echo "C2P $<"
	@scripts/cuf2pep.py < $< > $@

%.pt : %.ll_net
	@echo "P2PT $<"
	@src/pep2pt $< > $@

#%.ll_net : %.pt
#	@echo "PT2P $<"
#	@scripts/pt2pep.py < $< > $@

#%.ll_net : %.mp
#	@echo "C2P $<"
#	@scripts/mp2pep.py < $< > $@

%.unf.cuf : %.ll_net
	@echo "UNF $<"
	@src/cunf/cunf $< --save $@

%.mp.mp : %.unf.cuf
	@echo "MER $<"
	@scripts/cmerge.py < $< > $@

%.unf.cuf.tr : %.ll_net
	scripts/trt.py timeout=5000 t=cunf net=$< > $@

%.unf.mci : %.ll_net
	@echo "MLE $<"
	@mole $< -m $@

%.unf.mci.tr : %.ll_net
	scripts/trt.py timeout=900 t=mole net=$< > $@

%.dl.smod.tr : %.unf.mci.tr
	scripts/trt.py timeout=1200 t=dl.smod mci=$(<:%.tr=%) > $@

%.dl.cnmc.tr : %.unf.cuf.tr
	scripts/trt.py timeout=900 reps=10 t=dl.cnmc cnf cuf=$(<:%.tr=%) > $@

%.dl.cndc.tr : %.unf.cuf.tr
	scripts/trt.py timeout=900 reps=10 t=dl.cndc cuf=$(<:%.tr=%) > $@

%.dl.clp.tr : %.unf.mci.tr
	scripts/trt.py timeout=900 reps=10 t=dl.clp mci=$(<:%.tr=%) > $@

%.dl.lola.tr : %.ll_net
	scripts/trt.py timeout=600 t=dl.lola net=$< > $@

%.dl.smv.tr : %.ll_net
	scripts/trt.py timeout=120 t=dl.smv net=$< > $@

%.dl.mcm.tr : %.unf.mci.tr
	scripts/trt.py timeout=600 t=dl.mcm mci=$(<:%.tr=%) > $@

%.ll_net : %.xml
	@echo "X2P $<"
	@scripts/xml2pep.pl < $< > $@

%.ll_net : %.pnml
	@echo "P2P $<"
	@scripts/pnml2pep.py < $< > $@

%.ll_net : %.grml
	@echo "G2P $<"
	@scripts/grml2pep.py < $< > $@

%.r : %.dot
	@echo "RS  $<"
	@scripts/rs.pl $< > $@

%.ll_net : %.g
	@echo "X2P $<"
	@petrify -ip < $< | scripts/stg2pep.py > $@

%.dot : %.mci
	@echo "M2D $<"
	@mci2dot $< > $@

%.punf.r.c.txt : %.ll_net
	-punf -r -c -n=200000 -N=1 -s -t -@4 '-#' $< > $@ 2>&1
	echo >> $@
	echo "mci2mp:" >> $@
	mci2mp $(basename $<).mci >> $@
	echo >> $@
	echo "prcompress:" >> $@
	./prcompress -v $(basename $<).mci >> $@
	./scripts/cmerge.py < $(basename $<).pr.cuf > /dev/null 2>> $@

%.punf.c.txt : %.ll_net
	-punf -c -n=200000 -N=1 -s -t -@4 '-#' $< > $@ 2>&1
	echo >> $@
	echo "mci2mp:" >> $@
	mci2mp $(basename $<).mci >> $@

# dependency files
DEPS:=$(patsubst %.o,%.d,$(OBJS) $(MOBJS))
$(DEPS) : config.h
-include $(DEPS)

