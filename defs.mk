
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

# list of nets for several tasks
TEST_NETS:=$(shell tools/nets.sh test)
TIME_NETS:=$(shell tools/nets.sh time)
ALL_NETS:=$(shell tools/nets.sh no-huge)
MCI_NETS:=$(shell tools/nets.sh mci)
DEAD_NETS:=$(MCI_NETS)

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
	@#src/main $< | grep -C 17 cutoffs
	@#src/main $<
	@src/main $< > /dev/null

%.unf.mci : %.ll_net
	@echo "MLE $<"
	@mole $< -m $@

%.dl-smod : %.unf.mci
	@mcsmodels $< | grep 'FALSE\|TRUE' | \
		sed 's/FALSE/DEAD/; s/TRUE/LIVE/' | \
		(read R; /bin/echo -e "$$R\t$@")

%.dl-cnmc : %.unf.cuf
	@tools/cnmc.py dl $< | grep 'DEAD\|LIVE' | \
		sed 's/result.//' | \
		(read R; /bin/echo -e "$$R\t$@")

%.dl-clp : %.ll_net
	@check pep:clp-dl $< | grep 'YES\|NO' | \
		sed 's/Result: YES./LIVE/; s/Result: NO./DEAD/' | \
		(read R; /bin/echo -e "$$R\t$@")

%.dl-time-smod : %.unf.mci
	@tools/time.sh tools/time-mcsmodels.pl $<

%.dl-time-cnmc : %.unf.cuf
	@tools/time.sh tools/cnmc.py dl $<

%.dl-time-clp : %.ll_net
	@tools/time.sh 'check pep:clp-dl $< | (grep "Time needed"; /bin/echo -e "net\t$<") | sed "s/Time.*: /time\t/; s/ seconds//; s/000$$//"'

%.cnf : %.bc
	bc2cnf -all < $< > $@

%.bc : %.cuf
	tools/cnmc.py dl print $< 2> $@

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
