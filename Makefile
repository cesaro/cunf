
-include .config
include defs.mk

.PHONY: fake all menuconfig m g clean distclean

all: $(TARGETS)

fake :
	@echo $(CROSS)
	@echo $(CC)
	@echo $(SRCS)
	@echo $(MSRCS)
	@echo $(DEPS)

$(TARGETS) : % : %.o $(OBJS)
	@echo " LD  $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) 

e : $(TARGETS)
	./src/main test/examples/cont/pep/sdl_arq.ll_net

_e : $(TARGETS)
	./src/main test/examples/norm/pep/sem.ll_net
	./src/main test/examples/norm/pep/elevator.ll_net
	./src/main test/examples/norm/pep/do_od.ll_net
	./src/main test/examples/norm/pep/peterson.ll_net
	./src/main test/examples/norm/pep/mutual.ll_net
	./src/main test/examples/norm/bench/buf100.ll_net
	./src/main test/examples/norm/pep/parrow.ll_net
	./src/main test/examples/norm/pep/reader_writer_2.ll_net

	./src/main test/examples/cont/pep/do_od.ll_net
	./src/main test/examples/cont/pep/peterson_pfa.ll_net
	./src/main test/examples/cont/pep/peterson.ll_net
	./src/main test/examples/cont/pep/mutual.ll_net

	./src/main test/examples/small/asymcnfl.ll_net
	./src/main test/examples/small/conc.ll_net
	./src/main test/examples/small/test1.ll_net
	./src/main test/examples/small/fig2.ll_net
	./src/main test/examples/small/pag9.ll_net
	./src/main test/examples/small/fig1.ll_net
	./src/main test/examples/small/fig1-cp.ll_net
	./src/main test/examples/small/fig1-pr.ll_net
	./src/main test/examples/small/erv-size-parih.ll_net
	./src/main test/examples/small/erv-foata.ll_net
	./src/main test/examples/small/mcmillan-erv.ll_net
	./src/main test/examples/small/2readers.ll_net
	./src/main test/examples/small/8readers.ll_net
	./src/main test/examples/small/peupdate.ll_net
	./src/main test/examples/small/peupdate-bug.ll_net

	./src/main test/examples/norm/bench/ds_1.sync.ll_net
	./src/main test/examples/norm/bench/buf100.ll_net
	./src/main test/examples/norm/bench/byzagr4_1b.ll_net
	./src/main test/examples/norm/bench/pd_7.sync.ll_net
	./src/main test/examples/norm/bench/dph_7.dlmcs.ll_net
	./src/main test/examples/norm/bench/elevator_4.ll_net
	./src/main test/examples/norm/bench/elevator_4.mci
	./src/main test/examples/norm/bench/fifo20.ll_net
	./src/main test/examples/norm/bench/fifo20.mci
	./src/main test/examples/norm/bench/ftp_1.sync.ll_net
	./src/main test/examples/norm/bench/furnace_3.ll_net
	./src/main test/examples/norm/bench/key_3.sync.ll_net
	./src/main test/examples/norm/bench/key_4.fsa.ll_net
	./src/main test/examples/norm/bench/q_1.ll_net
	./src/main test/examples/norm/bench/q_1.sync.ll_net
	./src/main test/examples/norm/bench/rw_12.ll_net
	./src/main test/examples/norm/bench/rw_12.sync.ll_net
	./src/main test/examples/norm/bench/rw_1w3r.ll_net
	./src/main test/examples/norm/bench/rw_2w1r.ll_net

gp : $(TARGETS)
	./src/main test/examples/cont/pep/reader_writer_2.ll_net
	mv gmon.out gmon.out.1
	./src/main test/examples/cont/pep/parrow.ll_net
	mv gmon.out gmon.out.2
	#./src/main test/examples/cont/bench/buf100.ll_net
	./src/main test/examples/cont/pep/ab_gesc.ll_net
	mv gmon.out gmon.out.3
	gprof src/main gmon.out.* > s

ee : $(TARGETS)
	./src/main 2>&1 | tee /tmp/mole.log

g : $(TARGETS)
	gdb ./src/main


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

-include $(DEPS)

