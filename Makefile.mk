# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == Make ==
all:		# Default Rule
MAKEFLAGS      += -r
SHELL         ::= /bin/bash -o pipefail
PARALLEL_MAKE   = $(if $(filter -j, $(MFLAGS)),Yes,)
.PHONY:	FORCE
FORCE: ;

# == Basics ==
ALL_TARGETS	::=
CHECK_TARGETS	::=
CLEANFILES	::=
CLEANDIRS	::=
OUTDIR		 ?= out
>		::= $(OUTDIR)
.PHONY: ;
include config-utils.mk
include config-uname.mk

# == System Checks ==
PERL			?= perl
PYTHON2			?= python2.7
YAPPS			?= $(PYTHON2) $(abspath yapps2_deb/yapps2.py)
PKG_CONFIG		?= pkg-config
GLIB_MKENUMS		?= glib-mkenums
GDK_PIXBUF_CSOURCE	?= gdk-pixbuf-csource
PANDOC			?= pandoc
include config-checks.mk

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
clean:
	@test -z "$(strip $(CLEANFILES))" || (set -x; rm -f $(CLEANFILES) )
	@test -z "$(strip $(CLEANDIRS))" || (set -x; rm -fr $(CLEANDIRS) )
all: $(ALL_TARGETS)
check: $(CHECK_TARGETS)
$(CHECK_TARGETS): FORCE
