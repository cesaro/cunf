
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

clean :
	@rm -f $(TARGETS) $(MOBJS) $(OBJS)
	@echo Cleaning done.

distclean : clean
	@rm -f $(DEPS)
	@rm -f test/nets/tiny/*.r
	@rm -f test/nets/tiny/*.cuf
	@rm -f test/nets/tiny/*.dot
	@rm -f test/nets/tiny/*.pdf
	
	@rm -f test/nets/other/*.r
	@rm -f test/nets/other/*.cuf
	@rm -f test/nets/other/*.dot
	@rm -f test/nets/other/*.pdf
	
	@rm -f test/nets/cont/small/*.r
	@rm -f test/nets/cont/small/*.cuf
	@rm -f test/nets/cont/small/*.dot
	@rm -f test/nets/cont/small/*.pdf
	@rm -f test/nets/cont/med/*.r
	@rm -f test/nets/cont/med/*.cuf
	@rm -f test/nets/cont/med/*.dot
	@rm -f test/nets/cont/med/*.pdf
	@rm -f test/nets/cont/large/*.r
	@rm -f test/nets/cont/large/*.cuf
	@rm -f test/nets/cont/large/*.dot
	@rm -f test/nets/cont/large/*.pdf
	
	@rm -f test/nets/plain/small/*.r
	@rm -f test/nets/plain/small/*.cuf
	@rm -f test/nets/plain/small/*.dot
	@rm -f test/nets/plain/small/*.pdf
	@rm -f test/nets/plain/med/*.r
	@rm -f test/nets/plain/med/*.cuf
	@rm -f test/nets/plain/med/*.dot
	@rm -f test/nets/plain/med/*.pdf
	@rm -f test/nets/plain/large/*.r
	@rm -f test/nets/plain/large/*.cuf
	@rm -f test/nets/plain/large/*.dot
	@rm -f test/nets/plain/large/*.pdf
	
	@rm -f test/nets/pr/small/*.r
	@rm -f test/nets/pr/small/*.cuf
	@rm -f test/nets/pr/small/*.dot
	@rm -f test/nets/pr/small/*.pdf
	@rm -f test/nets/pr/med/*.r
	@rm -f test/nets/pr/med/*.cuf
	@rm -f test/nets/pr/med/*.dot
	@rm -f test/nets/pr/med/*.pdf
	@rm -f test/nets/pr/large/*.r
	@rm -f test/nets/pr/large/*.cuf
	@rm -f test/nets/pr/large/*.dot
	@rm -f test/nets/pr/large/*.pdf
	@echo Mr. Proper done.

-include $(DEPS)

