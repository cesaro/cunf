
-include .config
include defs.mk

.PHONY: fake all menuconfig m g test clean distclean testclean

all: $(TARGETS)

fake :
	@echo $(CC)
	@echo $(SRCS)
	@echo $(MSRCS)
	@echo $(TEST_NETS)
	@echo $(TIME_NETS)
	@echo $(DEPS)

$(TARGETS) : % : %.o $(OBJS)
	@echo " LD  $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) 

e : $(TARGETS)
	./src/main test/nets/cont/pep/sdl_arq.ll_net

_e : $(TARGETS)
	./src/main test/nets/norm/pep/sem.ll_net
	./src/main test/nets/norm/pep/elevator.ll_net
	./src/main test/nets/norm/pep/do_od.ll_net
	./src/main test/nets/norm/pep/peterson.ll_net
	./src/main test/nets/norm/pep/mutual.ll_net
	./src/main test/nets/norm/bench/buf100.ll_net
	./src/main test/nets/norm/pep/parrow.ll_net
	./src/main test/nets/norm/pep/reader_writer_2.ll_net

	./src/main test/nets/cont/pep/do_od.ll_net
	./src/main test/nets/cont/pep/peterson_pfa.ll_net
	./src/main test/nets/cont/pep/peterson.ll_net
	./src/main test/nets/cont/pep/mutual.ll_net

	./src/main test/nets/small/asymcnfl.ll_net
	./src/main test/nets/small/conc.ll_net
	./src/main test/nets/small/test1.ll_net
	./src/main test/nets/small/fig2.ll_net
	./src/main test/nets/small/pag9.ll_net
	./src/main test/nets/small/fig1.ll_net
	./src/main test/nets/small/fig1-cp.ll_net
	./src/main test/nets/small/fig1-pr.ll_net
	./src/main test/nets/small/erv-size-parih.ll_net
	./src/main test/nets/small/erv-foata.ll_net
	./src/main test/nets/small/mcmillan-erv.ll_net
	./src/main test/nets/small/2readers.ll_net
	./src/main test/nets/small/8readers.ll_net
	./src/main test/nets/small/peupdate.ll_net
	./src/main test/nets/small/peupdate-bug.ll_net

	./src/main test/nets/norm/bench/ds_1.sync.ll_net
	./src/main test/nets/norm/bench/buf100.ll_net
	./src/main test/nets/norm/bench/byzagr4_1b.ll_net
	./src/main test/nets/norm/bench/pd_7.sync.ll_net
	./src/main test/nets/norm/bench/dph_7.dlmcs.ll_net
	./src/main test/nets/norm/bench/elevator_4.ll_net
	./src/main test/nets/norm/bench/elevator_4.mci
	./src/main test/nets/norm/bench/fifo20.ll_net
	./src/main test/nets/norm/bench/fifo20.mci
	./src/main test/nets/norm/bench/ftp_1.sync.ll_net
	./src/main test/nets/norm/bench/furnace_3.ll_net
	./src/main test/nets/norm/bench/key_3.sync.ll_net
	./src/main test/nets/norm/bench/key_4.fsa.ll_net
	./src/main test/nets/norm/bench/q_1.ll_net
	./src/main test/nets/norm/bench/q_1.sync.ll_net
	./src/main test/nets/norm/bench/rw_12.ll_net
	./src/main test/nets/norm/bench/rw_12.sync.ll_net
	./src/main test/nets/norm/bench/rw_1w3r.ll_net
	./src/main test/nets/norm/bench/rw_2w1r.ll_net

gp : $(TARGETS)
	./src/main test/nets/cont/pep/reader_writer_2.ll_net
	mv gmon.out gmon.out.1
	./src/main test/nets/cont/pep/parrow.ll_net
	mv gmon.out gmon.out.2
	#./src/main test/nets/cont/bench/buf100.ll_net
	./src/main test/nets/cont/pep/ab_gesc.ll_net
	mv gmon.out gmon.out.3
	gprof src/main gmon.out.* > s

ee : $(TARGETS)
	./src/main 2>&1 | tee /tmp/mole.log

g : $(TARGETS)
	gdb ./src/main

test : $(TEST_R) $(TEST_UNF_R)
	@echo " DIF ..."
	@echo > t.diff
	@for n in $(TEST_NETS:%.ll_net=%); do diff -Naur $$n.r $$n.unf.r >> t.diff; done; true;

times : $(TIME_NETS:%.ll_net=%.time)


menuconfig .config : rules.out
	@echo " CNF $<"
	@./tools/cml2/cmlconfigure.py -c -i .config -o .config
	@./tools/cml2/configtrans.py -h include/config.h .config
	@rm -f $(DEPS)

include/config.h : .config
	@echo " CNF $<"
	@./tools/cml2/configtrans.py -h include/config.h .config

clean :
	@rm -f $(TARGETS) $(MOBJS) $(OBJS)
	@echo Cleaning done.

distclean : clean
	@rm -f rules.out .config $R/include/config.h
	@rm -f $(DEPS)
	@echo Mr. Proper done.

testclean :
	@rm -f $(TEST_UNF_R) $(TIME_UNF_DOT)
	@echo Cleaning of test results done.

-include $(DEPS)

