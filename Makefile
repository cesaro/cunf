
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

prof : $(TARGETS)
	src/main test/nets/plain/bench/bds_1.sync.ll_net
	mv gmon.out gmon.out.1
	src/main test/nets/plain/bench/buf100.ll_net
	mv gmon.out gmon.out.2
	src/main test/nets/plain/bench/byzagr4_1b.ll_net
	mv gmon.out gmon.out.3
	src/main test/nets/plain/bench/dpd_7.sync.ll_net
	mv gmon.out gmon.out.4
	src/main test/nets/plain/bench/dph_7.dlmcs.ll_net
	mv gmon.out gmon.out.5
	src/main test/nets/plain/bench/elevator_4.ll_net
	mv gmon.out gmon.out.6
	src/main test/nets/plain/bench/fifo20.ll_net
	mv gmon.out gmon.out.7
	src/main test/nets/plain/bench/ftp_1.sync.ll_net
	mv gmon.out gmon.out.8
	src/main test/nets/plain/bench/furnace_3.ll_net
	mv gmon.out gmon.out.9
	src/main test/nets/plain/bench/key_3.sync.ll_net
	mv gmon.out gmon.out.10
	src/main test/nets/plain/bench/key_4.fsa.ll_net
	mv gmon.out gmon.out.11
	src/main test/nets/plain/bench/q_1.ll_net
	mv gmon.out gmon.out.12
	src/main test/nets/plain/bench/q_1.sync.ll_net
	mv gmon.out gmon.out.13
	src/main test/nets/plain/bench/rw_12.ll_net
	mv gmon.out gmon.out.14
	src/main test/nets/plain/bench/rw_12.sync.ll_net
	mv gmon.out gmon.out.15
	src/main test/nets/plain/bench/rw_1w3r.ll_net
	mv gmon.out gmon.out.16
	src/main test/nets/plain/bench/rw_2w1r.ll_net
	mv gmon.out gmon.out.17
	gprof src/main gmon.out.* > out

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

