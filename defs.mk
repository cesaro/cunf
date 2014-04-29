
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

# traditional variables
#CFLAGS:=-Wall -Wextra -std=c99 -O3
#CFLAGS:=-Wall -Wextra -std=c99 -pg
CFLAGS:=-Wall -Wextra -std=c99 -g
#CFLAGS:=-Wall -Wextra -std=c99
#CXXFLAGS:=-Wall -Wextra -std=c++11 -O3
CXXFLAGS:=-Wall -Wextra -std=c++11 -g
CPPFLAGS:=-I src/ -D_POSIX_C_SOURCE=200809L -D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS
LDFLAGS:=-dead_strip
#LDFLAGS:=

# source code
SRCS:=$(wildcard src/*.c src/*.cc src/*/*.c src/*/*.cc src/*/*/*.c src/*/*/*.cc)
SRCS:=$(filter-out %/cunf.cc, $(SRCS))
SRCS:=$(filter-out %/pep2dot.c, $(SRCS))
SRCS:=$(SRCS) src/cna/spec_lexer.cc src/cna/spec_parser.cc

# do not remove files generated by lex or bison once you generate them
.SECONDARY: src/cna/spec_lexer.cc src/cna/spec_parser.cc

# source code containing a main() function
MSRCS:=$(wildcard src/cunf/cunf.cc src/pep2dot.c)

# compilation targets
OBJS:=$(SRCS:.cc=.o)
OBJS:=$(OBJS:.c=.o)
MOBJS:=$(MSRCS:.cc=.o)
MOBJS:=$(MOBJS:.c=.o)
TARGETS:=$(MOBJS:.o=)

# dependency files
DEPS:=$(patsubst %.o,%.d,$(OBJS) $(MOBJS))

# list of nets for several tasks
TEST_NETS:=$(shell tools/nets.sh test)
TIME_NETS:=$(shell tools/nets.sh time)
ALL_NETS:=$(shell tools/nets.sh no-huge)
MCI_NETS:=$(shell tools/nets.sh mci)
DEAD_NETS:=$(MCI_NETS)
CNMC_NETS:=$(shell tools/nets.sh all | grep -v huge)

# define the toolchain
CROSS:=

LD:=$(CROSS)ld
CC:=$(CROSS)gcc
CXX:=$(CROSS)g++
CPP:=$(CROSS)cpp
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

%.cc : %.y
	@echo "YAC $<"
	@$(YACC) -o $@ $^

# cancelling gnu make builtin rules for lex/yacc to c
# http://ftp.gnu.org/old-gnu/Manuals/make-3.79.1/html_chapter/make_10.html#SEC104
%.c : %.l
%.c : %.y

%.o : %.c
	@echo "CC  $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.o : %.cc
	@echo "CXX $<"
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

%.pdf : %.dot
	@echo "DOT $<"
	@dot -T pdf < $< > $@

%.jpg : %.dot
	@echo "DOT $<"
	@dot -T jpg < $< > $@

%.dot : %.ll_net
	@echo "P2D $<"
	@src/pep2dot $< > $@
	@#tools/pep2dot.py < $< > $@

%.ll_net : %.cuf
	@echo "C2P $<"
	@tools/cuf2pep.py < $< > $@

#%.ll_net : %.mp
#	@echo "C2P $<"
#	@tools/mp2pep.py < $< > $@

%.unf.cuf : %.ll_net
	@echo "UNF $<"
	@src/main $<

%.mp.mp : %.unf.cuf
	@echo "MER $<"
	@tools/cmerge.py < $< > $@

%.unf.cuf.tr : %.ll_net
	tools/trt.py timeout=5000 t=cunf net=$< > $@

%.unf.mci : %.ll_net
	@echo "MLE $<"
	@mole $< -m $@

%.unf.mci.tr : %.ll_net
	tools/trt.py timeout=900 t=mole net=$< > $@

%.dl.smod.tr : %.unf.mci.tr
	tools/trt.py timeout=1200 t=dl.smod mci=$(<:%.tr=%) > $@

%.dl.cnmc.tr : %.unf.cuf.tr
	tools/trt.py timeout=900 reps=10 t=dl.cnmc cnf cuf=$(<:%.tr=%) > $@

%.dl.cndc.tr : %.unf.cuf.tr
	tools/trt.py timeout=900 reps=10 t=dl.cndc cuf=$(<:%.tr=%) > $@

%.dl.clp.tr : %.unf.mci.tr
	tools/trt.py timeout=900 reps=10 t=dl.clp mci=$(<:%.tr=%) > $@

%.dl.lola.tr : %.ll_net
	tools/trt.py timeout=600 t=dl.lola net=$< > $@

%.dl.smv.tr : %.ll_net
	tools/trt.py timeout=120 t=dl.smv net=$< > $@

%.dl.mcm.tr : %.unf.mci.tr
	tools/trt.py timeout=600 t=dl.mcm mci=$(<:%.tr=%) > $@

%.ll_net : %.xml
	@echo "X2P $<"
	@tools/xml2pep.pl < $< > $@

%.ll_net : %.pnml
	@echo "P2P $<"
	@tools/pnml2pep.py < $< > $@

%.ll_net : %.grml
	@echo "G2P $<"
	@tools/grml2pep.py < $< > $@

%.r : %.dot
	@echo "RS  $<"
	@tools/rs.pl $< > $@

%.ll_net : %.g
	@echo "X2P $<"
	@petrify -ip < $< | tools/stg2pep.py > $@

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
	./tools/cmerge.py < $(basename $<).pr.cuf > /dev/null 2>> $@

%.punf.c.txt : %.ll_net
	-punf -c -n=200000 -N=1 -s -t -@4 '-#' $< > $@ 2>&1
	echo >> $@
	echo "mci2mp:" >> $@
	mci2mp $(basename $<).mci >> $@

