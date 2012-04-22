
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

include defs.mk

.PHONY: fake all g test clean distclean prof

all: $(TARGETS)

fake :
	@echo $(CC)
	@echo $(SRCS)
	@echo $(MSRCS)
	@echo $(TEST_NETS)
	@echo $(TIME_NETS)
	@echo $(DEAD_NETS)
	@echo $(DEPS)

$(TARGETS) : % : %.o $(OBJS)
	@echo "LD  $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) 

prof : $(TARGETS)
	rm gmon.out.*
	src/main /tmp/ele4.ll_net

g : $(TARGETS)
	gdb ./src/main

test tests : $(TEST_NETS:%.ll_net=%.r) $(TEST_NETS:%.ll_net=%.unf.r)
	@echo " DIF ..."
	@echo > t.diff
	@for n in $(TEST_NETS:%.ll_net=%); do diff -Na $$n.r $$n.unf.r >> t.diff; done; true;

cunf.tr : $(ALL_NETS:%.ll_net=%.unf.cuf.tr)
	@rm -f $@
	@cat $(ALL_NETS:%.ll_net=%.unf.cuf.tr) > $@

mole.tr : $(MCI_NETS:%.ll_net=%.unf.mci.tr)
	@rm -f $@
	@cat $(MCI_NETS:%.ll_net=%.unf.mci.tr) > $@

dl.smod.tr : $(DEAD_NETS:%.ll_net=%.dl.smod.tr)
	@rm -f $@
	@cat $(DEAD_NETS:%.ll_net=%.dl.smod.tr) > $@

dl.clp.tr : $(DEAD_NETS:%.ll_net=%.dl.clp.tr)
	@rm -f $@
	@cat $(DEAD_NETS:%.ll_net=%.dl.clp.tr) > $@

dl.cnmc.tr : $(CNMC_NETS:%.ll_net=%.dl.cnmc.tr)
	@rm -f $@
	@cat $(CNMC_NETS:%.ll_net=%.dl.cnmc.tr) > $@

dl.cndc.tr : $(CNMC_NETS:%.ll_net=%.dl.cndc.tr)
	@rm -f $@
	@cat $(CNMC_NETS:%.ll_net=%.dl.cndc.tr) > $@

dl.lola.tr : $(DEAD_NETS:%.ll_net=%.dl.lola.tr)
	@rm -f $@
	@cat $(DEAD_NETS:%.ll_net=%.dl.lola.tr) > $@

dl.smv.tr : $(DEAD_NETS:%.ll_net=%.dl.smv.tr)
	@rm -f $@
	@cat $(DEAD_NETS:%.ll_net=%.dl.smv.tr) > $@

dl.mcm.tr : $(DEAD_NETS:%.ll_net=%.dl.mcm.tr)
	@rm -f $@
	@cat $(DEAD_NETS:%.ll_net=%.dl.mcm.tr) > $@

clean :
	@rm -f $(TARGETS) $(MOBJS) $(OBJS)
	@echo Cleaning done.

distclean : clean
	@rm -f $(DEPS)
	@find test/nets/ -name '*.cnf' -exec rm '{}' ';'
	@find test/nets/ -name '*.mci' -exec rm '{}' ';'
	@find test/nets/ -name '*.bc' -exec rm '{}' ';'
	@find test/nets/ -name '*.r' -exec rm '{}' ';'
	@find test/nets/ -name '*.cuf' -exec rm '{}' ';'
	@find test/nets/ -name '*.dot' -exec rm '{}' ';'
	@find test/nets/ -name '*.pdf' -exec rm '{}' ';'
	@find test/nets/ -name '*.tr' -exec rm '{}' ';'
	@#rm -f test/nets/{plain,cont,pr}/{small,med,large,huge}/*.{cnf,mci,bc,r,cuf,dot,pdf}
	@echo Mr. Proper done.

-include $(DEPS)

