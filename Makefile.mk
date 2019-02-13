# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == Make ==
all:		# Default Rule
MAKEFLAGS += -r
SHELL    ::= /bin/bash -o pipefail

# == Basics ==
ALL_TARGETS ::=
CLEANFILES  ::=
CLEANDIRS   ::=
OUTDIR      ::= out
>	    ::= $(OUTDIR)
.PHONY: ;
include config-utils.mk
include config-uname.mk

# == Defaults ==
INCLUDES	::= -I.
DEFS		::=

# == Compiler Setup ==
DEBUGFLAGS       ?= -g
CXXSTD		::= -std=gnu++14 -pthread -pipe $(DEBUGFLAGS)
CSTD		::= -std=gnu11 -pthread -pipe $(DEBUGFLAGS)
EXTRA_DEFS	::= # target private defs, lesser precedence than CXXFLAGS
EXTRA_INCLUDES	::= # target private defs, lesser precedence than CXXFLAGS
EXTRA_FLAGS	::= # target private flags, precedence over CXXFLAGS
CCACHE		 ?= $(if $(CCACHE_DIR), ccache)

# == Rules ==
all: .PHONY
check: .PHONY
install: .PHONY
uninstall: .PHONY
clean: .PHONY

include config-checks.mk

# == implicit rules ==
$>/%/: ; $(Q) mkdir -p $@
$>/%.o: %.c
	$(QECHO) CC $@
	$(Q) $(CCACHE) $(CC) $(CSTD) -fPIC $(COMPILEDEFS) $(CFLAGS) $(COMPILEFLAGS) -o $@ -c $<
$>/%.o: %.cc
	$(QECHO) CXX $@
	$(Q) $(CCACHE) $(CXX) $(CXXSTD) -fPIC $(COMPILEDEFS) $(CXXFLAGS) $(COMPILEFLAGS) -o $@ -c $<
COMPILEDEFS  = $(DEFS) $(EXTRA_DEFS) $($<.DEFS) $($@.DEFS) $(INCLUDES) $(EXTRA_INCLUDES) $($<.INCLUDES) $($@.INCLUDES)
COMPILEFLAGS = $(EXTRA_FLAGS) $($<.FLAGS) $($@.FLAGS) -MQ '$@' -MMD -MF '$@'.d

# == rules ==
clean:
	@test -z "$(strip $(CLEANFILES))" || (set -x; rm -f $(CLEANFILES) )
	@test -z "$(strip $(CLEANDIRS))" || (set -x; rm -fr $(CLEANDIRS) )
all: $(ALL_TARGETS)
