
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
CFLAGS:=-Wall -Wextra -g
#CXXFLAGS:=-Wall -Wextra -O3 -std=c++0x
CXXFLAGS:=-Wall -Wextra -g -std=c++0x
CPPFLAGS:=-I include/
LDFLAGS:=-dead_strip
#LDFLAGS:=

# object file targets
SRCS:=$(wildcard src/*.c src/*.cc src/*/*.c src/*/*.cc)
SRCS:=$(filter-out %/main.cc, $(SRCS))
SRCS:=$(filter-out %/pep2dot.c, $(SRCS))

# object files containing a main() function
MSRCS:=$(wildcard src/main.cc src/pep2dot.c)

# compilation targets
OBJS:=$(patsubst %.cc,%.o,$(SRCS))
OBJS:=$(patsubst %.c,%.o,$(OBJS))
MOBJS:=$(patsubst %.cc,%.o,$(MSRCS))
MOBJS:=$(patsubst %.c,%.o,$(MOBJS))
TARGETS:=$(patsubst %.o,%,$(MOBJS))

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
#CC:=$(CROSS)gcc-fsf-4.5
CXX:=$(CROSS)g++
#CXX:=$(CROSS)g++-fsf-4.5
CPP:=$(CROSS)cpp

%.d : %.c
	@echo "DEP $<"
	@set -e; $(CC) -MM -MT $*.o $(CPPFLAGS) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@;

%.d : %.cc
	@echo "DEP $<"
	@set -e; $(CXX) -MM -MT $*.o $(CPPFLAGS) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@;

.c.o:
	@echo "CC  $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.cc.o:
	@echo "CXX $<"
	@$(CXX) $(CXXFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

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
