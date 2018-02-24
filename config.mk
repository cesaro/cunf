
# Define CONFIG_DEBUG to compile with debugging symbols and without optimization.
# Cunf will perform extensive assertions aiming at finding bugs in the code and
# corrupt data structures.  It will additionally dump verbose debugging
# information during the unfolding computation.  Switching on the option will
# considerably increase \cunf's running times. Define CONFIG_RELEASE to disable
# all the above and compile with -O3
CONFIG_DEBUG = 1
#CONFIG_RELEASE = 1

# Version of the tool
CONFIG_VERSION = v1.6.0

# Folder where the tool will be installed with "make install"
CONFIG_PREFIX = ~/x/local

# Define this macro to use McMillan's adequate order (see Ken McMillan's paper
# at CAV'92) when Cunf selects a possible extensions to extend the unfolding
# prefix.  One and only one of the macros
# - CONFIG_MCMILLAN
# - CONFIG_PARIKH
# - CONFIG_ERV
# - CONFIG_ERV_MOLE
# can be active.  Recall that McMillan's order is not a total order.
#CONFIG_MCMILLAN = 1

# Selects the 'Parikh' order of the Unfolding's book (Esparza, Heljanko
# 2008).  Although this this is an adequate order, it is not total, and may
# produce prefixes larger than any of the other available total orders.
# However, it always produces prefixes smaller than when using
# CONFIG_MCMILLAN.
#CONFIG_PARIKH = 1

# Selects the adequate order used in the Mole unfolder Mole.  This is a minor,
# non-documented modification of the order $\prec_F$.  It makes possible to
# compare the unfolding prefixes computed by Cunf and Mole.  Refer to the
# function h_cmp(), in src/h.c, for more details.
#CONFIG_ERV_MOLE = 1

# Selects the $\prec_F$ total adequate order the Esparza, Romer, Volger 2002 paper.
CONFIG_ERV = 1

# Defines the number of items to be allocated together whenever certain dynamic
# linked-lists need to grow.  Among other uses, these lists are used to store
# markings.  See src/nodelist.c
CONFIG_NODELIST_STEP = 1024

# Experimental. Don't use it.
# CONFIG_PMASK = 1

# Maximum verbosity level at which the tool produces output when requested to be
# verbose with --verb=N; touch these lines only if you know what you are doing.
ifdef CONFIG_DEBUG
CONFIG_MAX_VERB_LEVEL = 3
endif
ifdef CONFIG_RELEASE
CONFIG_MAX_VERB_LEVEL = 2
endif

########################

CONFIG_CFLAGS=$(CFLAGS)
CONFIG_COMPILE=$(COMPILE.c)
CONFIG_LINK=$(LINK.cc)
CONFIG_BUILD_DATE=$(shell date -R)
CONFIG_BUILD_COMMIT="$(shell git show --oneline | head -n1 | awk '{print $$1}')"
CONFIG_BUILD_DIRTY=$(shell if git diff-index --quiet HEAD --; then echo 0; else echo 1; fi)

ifdef CONFIG_DEBUG
ifdef CONFIG_RELEASE
$(error CONFIG_DEBUG and CONFIG_RELEASE are both defined, but are mutually exclusive)
endif
endif

ifneq ($(CONFIG_MCMILLAN)$(CONFIG_ERV)$(CONFIG_ERV_MOLE)$(CONFIG_PARIKH),1)
$(error Atmost one of CONFIG_{MCMILLAN,ERV,ERV_MOLE,PARIKH} can be defined!)
endif

CONFIGVARS=$(filter CONFIG_%,$(.VARIABLES))
export $(CONFIGVARS)

config.h : config.mk
	@echo "GEN $@"
	@scripts/env2h.py $(CONFIGVARS) > $@
