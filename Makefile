
-include .config
include defs.mk

.PHONY: fake all menuconfig m g clean distclean

all: $(TARGETS)

fake :
	@echo $(MSRCS)
	@echo $(CC)
	@echo $(SRCS)

$(TARGETS) : % : %.o $(OBJS)
	@echo " LD  $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) 

_e : $(TARGETS)
	./test/dg
	./src/main test/examples/small/test1.ll_net
	./src/main test/examples/small/fig1-pr.ll_net
	./src/main test/examples/small/fig2.ll_net
	./src/main test/examples/small/8readers.ll_net
	./src/main test/examples/small/pag9.ll_net
	./src/main test/examples/small/asymcnfl.ll_net
	./src/main test/examples/small/fig1.ll_net
	./src/main test/examples/small/fig1-cp.ll_net

e : $(TARGETS)
	./src/main test/examples/normal-nets/pep/sem.ll_net

t : $(TARGETS)
	time ./test/dg

ee : $(TARGETS)
	./test/dg 2>&1 | tee /tmp/mole.log

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

