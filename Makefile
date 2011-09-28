
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

.PHONY: fake all g test times clean distclean prof

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

test : $(TEST_R) $(TEST_UNF_R)
	@echo " DIF ..."
	@echo > t.diff
	@for n in $(TEST_NETS:%.ll_net=%); do diff -Na $$n.r $$n.unf.r >> t.diff; done; true;

times : $(TIME_NETS:%.ll_net=%.time)
dead1 : $(DEAD_NETS:%.ll_net=%.dead1)
dead2 : $(DEAD_NETS:%.ll_net=%.dead2)

clean :
	@rm -f $(TARGETS) $(MOBJS) $(OBJS)
	@echo Cleaning done.

distclean : clean
	@rm -f $(DEPS)
	@find test/nets/ -name '*.r' -exec rm '{}' ';'
	@find test/nets/ -name '*.cuf' -exec rm '{}' ';'
	@find test/nets/ -name '*.dot' -exec rm '{}' ';'
	@find test/nets/ -name '*.pdf' -exec rm '{}' ';'
	@echo Mr. Proper done.

-include $(DEPS)

