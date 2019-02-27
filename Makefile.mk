# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == MAKE setup ==
all:		# Default Rule
MAKEFLAGS      += -r
SHELL         ::= /bin/bash -o pipefail
PARALLEL_MAKE   = $(if $(filter -j, $(MFLAGS)),Yes,)
S ::= # Variable containing 1 space
S +=

# == User Defaults ==
# see also 'make default' rule
-include config-defaults.mk

# == Mode ==
# determine build mode
override MODE !=  case "$(MODE)" in \
		    p*|pr*|pro*|prod*|produ*|produc*|product*)		MODE=release ;; \
		    producti*|productio*|production)			MODE=release ;; \
		    r*|re*|rel*|rele*|relea*|releas*|release)		MODE=release ;; \
		    d*|de*|deb*|debu*|debug|dbg)			MODE=debug ;; \
		    dev*|deve*|devel*|develo*|develop*|developm*)	MODE=debug ;; \
		    developme*|developmen*|development)			MODE=debug ;; \
		    u*|ub*|ubs*|ubsa*|ubsan)				MODE=ubsan ;; \
		    a*|as*|asa*|asan)					MODE=asan ;; \
		    t*|ts*|tsa*|tsan)					MODE=tsan ;; \
		    l*|ls*|lsa*|lsan)					MODE=lsan ;; \
		    *)							MODE=debug ;; \
		  esac ; echo "$$MODE"
.config.defaults += MODE
$(info $S MODE     $(MODE))

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

# == builddir variants ==
>		  = $(builddir)
# if 'realpath --relative-to' is missing, os.path.relpath could be used as fallback
build2srcdir	 != realpath --relative-to $(builddir) .

# == Target Collections ==
ALL_TARGETS	::=
ALL_TESTS	::=
CHECK_TARGETS	::=
CLEANFILES	::=
CLEANDIRS	::=

# == Defaults ==
INCLUDES	::= -I.
DEFS		::=

# == Compiler Setup ==
CXXSTD		::= -std=gnu++14 -pthread -pipe
CSTD		::= -std=gnu11 -pthread -pipe
EXTRA_DEFS	::= # target private defs, lesser precedence than CXXFLAGS
EXTRA_INCLUDES	::= # target private defs, lesser precedence than CXXFLAGS
EXTRA_FLAGS	::= # target private flags, precedence over CXXFLAGS

# == Utilities & Checks ==
include config-utils.mk
include config-uname.mk
include config-checks.mk
.config.defaults += CC CFLAGS CXX CXXFLAGS LDFLAGS

# == enduser targets ==
all: FORCE
check: FORCE
install: FORCE
uninstall: FORCE

# == subdirs ==
include res/Makefile.mk
include media/Makefile.mk
include sfi/Makefile.mk
include aidacc/Makefile.mk
include bse/Makefile.mk
include plugins/Makefile.mk
include drivers/Makefile.mk
include tests/Makefile.mk
include ebeast/Makefile.mk
include beast-gtk/Makefile.mk

# == FORCE rules ==
# Use FORCE to mark phony targets via a dependency
.PHONY:	FORCE

# == output directory rules ==
# rule to create output directories from order only dependencies, trailing slash required
$>/%/:
	$Q mkdir -p $@
.PRECIOUS: $>/%/ # prevent MAKE's 'rm ...' for automatically created dirs

# == 'default' settings ==
# Allow value defaults to be adjusted via: make default builddir=... CXX=...
default: FORCE
	$(QECHO) WRITE config-defaults.mk
	$Q echo -e '# make $@\n'			> $@.tmp
	$Q : $(foreach VAR, $(.config.defaults),		  \
	       $(if $(filter command, $(origin $(VAR))),	  \
	  && echo '$(VAR)  ?= $(value $(VAR))'		>>$@.tmp, \
	  && echo '# $(VAR) = $(value $(VAR))'		>>$@.tmp  \
	      ) )
	$Q mv $@.tmp config-defaults.mk

# == clean rules ==
clean: FORCE
	@test -z "$(strip $(CLEANFILES))" || (set -x; rm -f $(CLEANFILES) )
	@test -z "$(strip $(CLEANDIRS))" || (set -x; rm -fr $(CLEANDIRS) )

# == help rules ==
help: FORCE
	@echo 'Make targets:'
	@: #   12345678911234567892123456789312345678941234567895123456789612345678971234567898
	@echo '  all             - Build all targets, uses config-defaults.mk'
	@echo '  clean           - Remove build directory, but keeps config-defaults.mk'
	@echo '  default         - Write variable defaults into config-defaults.mk, these can'
	@echo '                    be overridden by MAKE command line variables; look at this'
	@echo '                    file for a list of variables that can be customized'
	@echo '  check           - Run selft tests and unit tests'
	@echo 'Invocation:'
	@echo '  make V=1        - Enable verbose output from MAKE and subcommands'

# == all rules ==
all: $(ALL_TARGETS) $(ALL_TESTS)

# == check rules ==
check: FORCE
# Macro to generate test runs as 'check' dependencies
define CHECK_ALL_TESTS_TEST
CHECK_TARGETS += $$(dir $1)check-$$(notdir $1)
$$(dir $1)check-$$(notdir $1): $1
	$$(QECHO) RUN… $$@
	$$Q $1
endef
$(foreach TEST, $(ALL_TESTS), $(eval $(call CHECK_ALL_TESTS_TEST, $(TEST))))
check: $(CHECK_TARGETS)
$(CHECK_TARGETS): FORCE
