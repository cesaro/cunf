
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

.PHONY: fake all g test times clean distclean testclean

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

e : $(TARGETS)
	./src/main test/nets/cont/pep/sdl_arq.ll_net

prof : $(TARGETS)
	rm gmon.out.*
	src/main /tmp/ele4.ll_net
	mv gmon.out gmon.out.1
	src/main /tmp/ele4.ll_net
	mv gmon.out gmon.out.2
	src/main /tmp/ele4.ll_net
	mv gmon.out gmon.out.3
	src/main /tmp/ele4.ll_net
	mv gmon.out gmon.out.4
	src/main /tmp/ele4.ll_net
	mv gmon.out gmon.out.5
	src/main /tmp/ele4.ll_net
	mv gmon.out gmon.out.6
	src/main /tmp/ele4.ll_net
	mv gmon.out gmon.out.7
	src/main /tmp/ele4.ll_net
	mv gmon.out gmon.out.8
	src/main /tmp/ele4.ll_net
	mv gmon.out gmon.out.9
	src/main /tmp/ele4.ll_net
	mv gmon.out gmon.out.10

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
	@echo Mr. Proper done.

testclean :
	@#rm -f $(TEST_R) $(TEST_UNF_R)
	@rm -f $(TEST_UNF_R)
	@echo Cleaning of test results done.

allclean :
	rm -f test/nets/small/*.r
	rm -f test/nets/small/*.dot
	rm -f test/nets/small/*.pdf
	
	rm -f test/nets/cont/pep/*.r
	rm -f test/nets/cont/pep/*.dot
	rm -f test/nets/cont/pep/*.pdf
	rm -f test/nets/cont/bench/*.r
	rm -f test/nets/cont/bench/*.dot
	rm -f test/nets/cont/bench/*.pdf
	
	rm -f test/nets/plain/pep/*.r
	rm -f test/nets/plain/pep/*.dot
	rm -f test/nets/plain/pep/*.pdf
	rm -f test/nets/plain/bench/*.r
	rm -f test/nets/plain/bench/*.dot
	rm -f test/nets/plain/bench/*.pdf
	
	rm -f test/nets/pr/pep/*.r
	rm -f test/nets/pr/pep/*.dot
	rm -f test/nets/pr/pep/*.pdf
	rm -f test/nets/pr/bench/*.r
	rm -f test/nets/pr/bench/*.dot
	rm -f test/nets/pr/bench/*.pdf

-include $(DEPS)

