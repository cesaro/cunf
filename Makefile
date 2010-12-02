
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
	@echo " LD  $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) 

e : $(TARGETS)
	./src/main test/nets/cont/pep/sdl_arq.ll_net

_e : $(TARGETS)
	./src/main test/nets/plain/pep/sem.ll_net
	./src/main test/nets/plain/pep/elevator.ll_net
	./src/main test/nets/plain/pep/do_od.ll_net
	./src/main test/nets/plain/pep/peterson.ll_net
	./src/main test/nets/plain/pep/mutual.ll_net
	./src/main test/nets/plain/bench/buf100.ll_net
	./src/main test/nets/plain/pep/parrow.ll_net
	./src/main test/nets/plain/pep/reader_writer_2.ll_net

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

	./src/main test/nets/plain/bench/buf100.ll_net
	./src/main test/nets/plain/bench/byzagr4_1b.ll_net
	./src/main test/nets/plain/bench/dph_7.dlmcs.ll_net
	./src/main test/nets/plain/bench/elevator_4.ll_net
	./src/main test/nets/plain/bench/elevator_4.mci
	./src/main test/nets/plain/bench/fifo20.ll_net
	./src/main test/nets/plain/bench/fifo20.mci
	./src/main test/nets/plain/bench/ftp_1.sync.ll_net
	./src/main test/nets/plain/bench/furnace_3.ll_net
	./src/main test/nets/plain/bench/key_3.sync.ll_net
	./src/main test/nets/plain/bench/key_4.fsa.ll_net
	./src/main test/nets/plain/bench/q_1.ll_net
	./src/main test/nets/plain/bench/q_1.sync.ll_net
	./src/main test/nets/plain/bench/rw_12.ll_net
	./src/main test/nets/plain/bench/rw_12.sync.ll_net
	./src/main test/nets/plain/bench/rw_1w3r.ll_net
	./src/main test/nets/plain/bench/rw_2w1r.ll_net

gp : $(TARGETS)
	./src/main test/nets/cont/pep/reader_writer_2.ll_net
	mv gmon.out gmon.out.1
	./src/main test/nets/cont/pep/parrow.ll_net
	mv gmon.out gmon.out.2
	#./src/main test/nets/cont/bench/buf100.ll_net
	./src/main test/nets/cont/pep/ab_gesc.ll_net
	mv gmon.out gmon.out.3
	gprof src/main gmon.out.* > s

g : $(TARGETS)
	gdb ./src/main

test : $(TEST_R) $(TEST_UNF_R)
	@echo " DIF ..."
	@echo > t.diff
	@for n in $(TEST_NETS:%.ll_net=%); do diff -Naur $$n.r $$n.unf.r >> t.diff; done; true;

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

-include $(DEPS)

