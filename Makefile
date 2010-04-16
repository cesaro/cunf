
-include .config
include defs.mk

.PHONY: fake all menuconfig m g clean distclean

all: $(TARGETS)

fake :
	@echo $(MSRCS)
	@echo $(CC)

$(TARGETS) : % : %.o $(OBJS)
	@echo " LD  $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) 

e : $(TARGETS)
	./test/dg

t : $(TARGETS)
	time ./test/dg

ee : $(TARGETS)
	./test/dg 2>&1 | tee /tmp/mole.log

g : $(TARGETS)
	gdb ./test/dg

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

