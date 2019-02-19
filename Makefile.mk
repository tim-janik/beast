# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == Make ==
all:		# Default Rule
MAKEFLAGS      += -r
SHELL         ::= /bin/bash -o pipefail
PARALLEL_MAKE   = $(if $(filter -j, $(MFLAGS)),Yes,)
.PHONY:	FORCE
FORCE: ;

# == User Defaults ==
# see also 'make defaults' rule
-include config-defaults.mk

# == Dirctories ==
builddir	 ?= out
prefix		 ?= /usr/local
bindir		 ?= $(prefix)/bin
datadir 	 ?= $(prefix)/share
mandir		 ?= $(datadir)/man
libdir		 ?= $(prefix)/lib
pkgrootdir	 ?= $(libdir)
pkglibdir	 ?= $(pkgrootdir)/beast-$(VERSION_MAJOR)-$(VERSION_MINOR)
.config.defaults += builddir prefix bindir datadir mandir libdir pkgrootdir pkglibdir
>		 = $(builddir)

# == Basic Setup & Checks ==
ALL_TARGETS	::=
CHECK_TARGETS	::=
CLEANFILES	::=
CLEANDIRS	::=
include config-utils.mk
include config-uname.mk
include config-checks.mk
.config.defaults += CC CFLAGS CXX CXXFLAGS LDFLAGS

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

# == Rules ==
all: FORCE
check: FORCE
install: FORCE
uninstall: FORCE
clean: FORCE

# == subdirs ==
include res/Makefile.mk
include sfi/Makefile.mk
include aidacc/Makefile.mk
include bse/Makefile.mk
include plugins/Makefile.mk
include drivers/Makefile.mk
include tests/Makefile.mk
include ebeast/Makefile.mk

# == output directories ==
$>/%/: ; $(Q) mkdir -p $@
.PRECIOUS: $>/%/ # prevent MAKE's 'rm ...' for automatically created dirs

# == rules ==
# Allow value defaults to be adjusted via: make config builddir=... CXX=...
defaults: FORCE
	$(QECHO) WRITE config-defaults.mk
	$Q echo -e '# make $@\n'			> $@.tmp
	$Q : $(foreach VAR, $(.config.defaults),		  \
	       $(if $(filter command, $(origin $(VAR))),	  \
	  && echo '$(VAR)  ?= $(value $(VAR))'		>>$@.tmp, \
	  && echo '# $(VAR) = $(value $(VAR))'		>>$@.tmp  \
	      ) )
	$Q mv $@.tmp config-defaults.mk
clean:
	@test -z "$(strip $(CLEANFILES))" || (set -x; rm -f $(CLEANFILES) )
	@test -z "$(strip $(CLEANDIRS))" || (set -x; rm -fr $(CLEANDIRS) )
help: FORCE
	@echo 'Make targets:'
	@: #   12345678911234567892123456789312345678941234567895123456789612345678971234567898
	@echo '  all             - Build all targets, uses config-defaults.mk'
	@echo '  clean           - Remove build directory, but keeps config-defaults.mk'
	@echo '  defaults        - Write variable defaults into config-defaults.mk, these can'
	@echo '                    be overridden by MAKE command line variables; look at this'
	@echo '                    file for a list of variables that can be customized'
	@echo '  check           - Run selft tests and unit tests'
	@echo 'Invocation:'
	@echo '  make V=1        - Enable verbose output from MAKE and subcommands'
all: $(ALL_TARGETS)
check: $(CHECK_TARGETS)
$(CHECK_TARGETS): FORCE
