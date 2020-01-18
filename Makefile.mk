# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == MAKE setup ==
all:		# Default Rule
MAKEFLAGS      += -r
SHELL         ::= /bin/bash -o pipefail
PARALLEL_MAKE   = $(filter JOBSERVER, $(subst -j, JOBSERVER , $(MFLAGS)))
S ::= # Variable containing 1 space
S +=

# == User Defaults ==
# see also 'make default' rule
-include config-defaults.mk

# == Mode ==
# determine build mode
MODE.origin ::= $(origin MODE) # before overriding, remember if MODE came from command line
override MODE !=  case "$(MODE)" in \
		    p*|pr*|pro*|prod*|produ*|produc*|product*)		MODE=production ;; \
		    producti*|productio*|production)			MODE=production ;; \
		    r*|re*|rel*|rele*|relea*|releas*|release)		MODE=production ;; \
		    d*|de*|deb*|debu*|debug|dbg)			MODE=debug ;; \
		    dev*|deve*|devel*|develo*|develop*|developm*)	MODE=debug ;; \
		    developme*|developmen*|development)			MODE=debug ;; \
		    u*|ub*|ubs*|ubsa*|ubsan)				MODE=ubsan ;; \
		    a*|as*|asa*|asan)					MODE=asan ;; \
		    t*|ts*|tsa*|tsan)					MODE=tsan ;; \
		    l*|ls*|lsa*|lsan)					MODE=lsan ;; \
		    q*|qu*|qui*|quic*|quick)				MODE=quick ;; \
		    *)							MODE=production ;; \
		  esac ; echo "$$MODE"
.config.defaults += MODE
$(info $S MODE     $(MODE))

# == builddir ==
# Allow O= and builddir= on the command line
ifeq ("$(origin O)", "command line")
builddir	::= $O
builddir.origin ::= command line
endif
ifeq ('$(builddir)', '')
builddir	::= out
endif
# Provide $> as builddir shorthand, used in almost every rule
>		  = $(builddir)
# if 'realpath --relative-to' is missing, os.path.relpath could be used as fallback
build2srcdir	 != realpath --relative-to $(builddir) .

# == Dirctories ==
prefix		 ?= /usr/local
bindir		 ?= $(prefix)/bin
datadir 	 ?= $(prefix)/share
mandir		 ?= $(datadir)/man
libdir		 ?= $(prefix)/lib
pkgrootdir	 ?= $(libdir)
pkglibdir	 ?= $(pkgrootdir)/beast-$(VERSION_MAJOR)-$(VERSION_MINOR)
pkgsharedir	 ?= $(pkglibdir)/share
.config.defaults += prefix bindir datadir mandir libdir pkgrootdir pkglibdir

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
CXXSTD		::= -std=gnu++17 -pthread -pipe
CSTD		::= -std=gnu11 -pthread -pipe
EXTRA_DEFS	::= # target private defs, lesser precedence than CXXFLAGS
EXTRA_INCLUDES	::= # target private defs, lesser precedence than CXXFLAGS
EXTRA_FLAGS	::= # target private flags, precedence over CXXFLAGS

# == Utilities & Checks ==
include config-utils.mk
include config-uname.mk
include config-checks.mk
.config.defaults += CC CFLAGS CXX CXXFLAGS LDFLAGS LDLIBS

# == enduser targets ==
all: FORCE
check: FORCE
check-audio: FORCE
install: FORCE
uninstall: FORCE
installcheck: FORCE

# == subdirs ==
include res/Makefile.mk
include images/Makefile.mk
include po/Makefile.mk
include data/Makefile.mk
include media/Makefile.mk
include sfi/Makefile.mk
include aidacc/Makefile.mk
include bse/Makefile.mk
include plugins/Makefile.mk
include tools/Makefile.mk
include tests/Makefile.mk
include ebeast/Makefile.mk
include launchers/Makefile.mk
include docs/Makefile.mk
include misc/Makefile.mk

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
	$Q echo -e '# make $@\n'					> $@.tmp
	$Q echo 'builddir = $(builddir)'				>>$@.tmp
	$Q : $(foreach VAR, $(.config.defaults), &&					\
	  if $(if $(filter command, $(origin $(VAR)) $($(VAR).origin)),			\
		true, false) ; then							\
	    echo '$(VAR) = $(value $(VAR))'				>>$@.tmp ;	\
	  elif ! grep -sEm1 '^$(VAR)\s*:?[:!?]?=' config-defaults.mk	>>$@.tmp ; then	\
	    echo '# $(VAR) = $(value $(VAR))'				>>$@.tmp ;	\
	  fi )
	$Q mv $@.tmp config-defaults.mk

# == run ==
run: FORCE ebeast/run

# == clean rules ==
clean: FORCE
	@test -z "$(strip $(CLEANFILES))" || (set -x; rm -f $(CLEANFILES) )
	@test -z "$(strip $(CLEANDIRS))" || (set -x; rm -fr $(CLEANDIRS) )

# == help rules ==
help: FORCE
	@echo 'Make targets:'
	@: #   12345678911234567892123456789312345678941234567895123456789612345678971234567898
	@echo '  all             - Build all targets, uses config-defaults.mk if present.'
	@echo '  clean           - Remove build directory, but keeps config-defaults.mk.'
	@echo '  install         - Install binaries and data files under $$(prefix)'
	@echo '  uninstall       - Uninstall binaries, aliases and data files'
	@echo '  installcheck    - Run checks on the installed project files.'
	@echo '  default         - Create config-defaults.mk with variables set via the MAKE'
	@echo '                    command line. Inspect the file for a list of variables to'
	@echo '                    be customized. Deleting it will undo any customizations.'
	@echo '  check           - Run selfttests and unit tests'
	@echo '  check-audio     - Validate BSE rendering against reference files'
	@echo '  check-x11       - Optional checks that are skipped without $$DISPLAY'
	@echo '  check-bench     - Run the benchmark tests'
	@echo '  check-loading   - Check all distributed BSE files load properly'
	@echo '  check-suite     - Run the unit test suite'
	@echo 'Invocation:'
	@echo '  make V=1        - Enable verbose output from MAKE and subcommands'
	@echo '  make O=DIR      - Create all output files in DIR, see also config-defaults.mk'
	@echo '                    for related variables like CXXFLAGS'
	@echo '  make DESTDIR=/  - Absolute path prepended to all install/uninstall locations'
	@echo "  make MODE=...   - Optimize build to be 'quick' or for 'production' mode binaries."
	@echo '                    Posible modes for debugging: debug, asan, lsan, tsan, ubsan'

# == all rules ==
all: $(ALL_TARGETS) $(ALL_TESTS)

# == check rules ==
# Macro to generate test runs as 'check' dependencies
define CHECK_ALL_TESTS_TEST
CHECK_TARGETS += $$(dir $1)check-$$(notdir $1)
$$(dir $1)check-$$(notdir $1): $1
	$$(QECHO) RUN… $$@
	$$Q $1
endef
$(foreach TEST, $(ALL_TESTS), $(eval $(call CHECK_ALL_TESTS_TEST, $(TEST))))
check: $(CHECK_TARGETS) check-audio check-bench
$(CHECK_TARGETS): FORCE
check-bench: FORCE
check-x11 check-x11-disabled: FORCE
ifneq ($(DISPLAY),)
check: check-x11
else
check: check-x11-disabled
check-x11-disabled:
	@echo '  SKIP    ' 'Tests that involve an X11 environment with $$DISPLAY'
endif

# == installcheck ==
installcheck-buildtest:
	$(QGEN)
	$Q cd $> $(file > $>/conftest_buildtest.cc, $(conftest_buildtest.c)) \
	&& test -r conftest_buildtest.cc \
		; X=$$? ; echo -n "Create  BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$Q cd $> \
	&& $(CCACHE) $(CXX) $(CXXSTD) -Werror \
		`PKG_CONFIG_PATH="$(DESTDIR)$(pkglibdir)/lib/pkgconfig:$(libdir)/pkgconfig:$$PKG_CONFIG_PATH" $(PKG_CONFIG) --cflags bse` \
		-c conftest_buildtest.cc \
		; X=$$? ; echo -n "Compile BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$Q cd $> \
	&& $(CCACHE) $(CXX) $(CXXSTD) -Werror conftest_buildtest.o -o conftest_buildtest $(LDMODEFLAGS) \
		`PKG_CONFIG_PATH="$(DESTDIR)$(pkglibdir)/lib/pkgconfig:$(libdir)/pkgconfig:$$PKG_CONFIG_PATH" $(PKG_CONFIG) --libs bse` \
		; X=$$? ; echo -n "Link    BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$Q cd $> \
	&& LD_LIBRARY_PATH="$(DESTDIR)$(pkglibdir)/lib:$$LD_LIBRARY_PATH" ./conftest_buildtest \
		; X=$$? ; echo -n "Execute BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$Q cd $> \
	&& rm -f conftest_buildtest.cc conftest_buildtest.o conftest_buildtest
.PHONY: installcheck-buildtest
installcheck: installcheck-buildtest
# conftest_buildtest.c
define conftest_buildtest.c
#include <bse/bse.hh>
extern "C"
int main (int argc, char *argv[])
{
  Bse::init_async (&argc, argv, "bse-app-test");
  return 0;
}
endef

# == dist ==
# eval: distversion != ./version.sh -l
distname     = beast-$(distversion)
disttarball  = $>/$(distname).tar.xz
dist_xz_opt  ?= -9e
dist: $>/doc/README $>/ChangeLog FORCE
	@$(eval distversion != ./version.sh -l)
	$(QECHO) MAKE $(disttarball)
	$Q DIFFLINES=`git diff HEAD | wc -l` \
	  && { test 0 = $$DIFFLINES || echo -e "#\n# $@: WARNING: working tree unclean\n#" >&2 ; }
	$Q git archive --format=tar --prefix=$(distname)/ HEAD		> $>/$(distname).tar
	$Q rm -rf $>/.extradist/ && mkdir -p $>/.extradist/$(distname)/	\
	  && : tar -C $>/.extradist/ -xf $>/$(distname).tar
	$Q cp -a $>/doc/README $>/ChangeLog $>/.extradist/$(distname)/
	$Q cd $>/.extradist/ && tar uhf $(abspath $>/$(distname).tar) $(distname)
	$Q rm -f -r $>/.extradist/
	$Q rm -f $>/$(distname).tar.xz && xz $(dist_xz_opt) $>/$(distname).tar && test -e $(disttarball)
	$Q echo "Archive ready: $(disttarball)" | sed '1h; 1s/./=/g; 1p; 1x; $$p; $$x'
CLEANFILES += $(wildcard $>/beast-*.tar $>/beast-*.tar.xz)

# == distcheck ==
# Distcheck aims:
# - build *outside* the original source tree to catch missing files or dirs, and without picking up parent directory contents;
# - support parallel builds;
# - verify that no CLEANFILES are shipped in dist tarball;
# - check that $(DESTDIR) is properly honored in installation rules.
# distcheck_uniqdir - directory for build tests, outside of srcdir, unique per user and checkout
# distcheck_uniqdir = distcheck-$(shell printf %d-%04x\\n $$UID 0x`X=$$(pwd) && echo -n "$$X" | md5sum | sed 's/^\(....\).*/\1/'`)
#distcheck: distcheck_uniqdir ::= distcheck-$(shell python -c "import os, md5; print ('%u-%s' % (os.getuid(), md5.new (os.getcwd()).hexdigest()[:4]))")
distcheck: distcheck_uniqdir ::= distcheck-beast-$(shell python -c "import os, md5; print ('%u-%s' % (os.getuid(), md5.new (os.getcwd()).hexdigest()[:5]))")
distcheck: dist
	$(QGEN)
	@$(eval distversion != ./version.sh -l)
	$Q TMPDIR="$${TMPDIR-$${TEMP-$${TMP-/tmp}}}" \
	&& DCDIR="$$TMPDIR/$(distcheck_uniqdir)" \
	&& test -n "$$TMPDIR" -a -n "$(distcheck_uniqdir)" -a -n "$$DCDIR" -a -n "$(distname)" \
	&& { test ! -e "$$DCDIR/" || { chmod u+w -R "$$DCDIR/" && rm -r "$$DCDIR/" ; } ; } \
	&& mkdir -p "$$DCDIR" \
	&& set -x \
	&& cd "$$DCDIR" \
	&& tar xf $(abspath $(disttarball)) \
	&& cd "$(distname)" \
	&& $(MAKE) default prefix="$$DCDIR/inst" \
	&& touch dc-buildtree-cleaned \
	&& find . ! -path './out/*' -print >dc-buildtree-files \
	&& $(MAKE) clean \
	&& find . ! -path './out/*' -print >dc-buildtree-cleaned \
	&& diff -u dc-buildtree-files dc-buildtree-cleaned \
	&& $(MAKE) $(AM_MAKEFLAGS) -j`nproc` \
	&& $(MAKE) $(AM_MAKEFLAGS) check \
	&& $(MAKE) $(AM_MAKEFLAGS) install \
	&& $(MAKE) $(AM_MAKEFLAGS) installcheck \
	&& $(MAKE) $(AM_MAKEFLAGS) uninstall \
	&& $(MAKE) $(AM_MAKEFLAGS) distuninstallcheck distuninstallcheck_dir="$$DCDIR/inst" \
	&& chmod a-w -R "$$DCDIR/inst" \
	&& mkdir -m 0700 "$$DCDIR/destdir" \
	&& $(MAKE) $(AM_MAKEFLAGS) DESTDIR="$$DCDIR/destdir" install \
	&& $(MAKE) $(AM_MAKEFLAGS) DESTDIR="$$DCDIR/destdir" uninstall \
	&& $(MAKE) $(AM_MAKEFLAGS) DESTDIR="$$DCDIR/destdir" distuninstallcheck distuninstallcheck_dir="$$DCDIR/destdir" \
	&& $(MAKE) $(AM_MAKEFLAGS) clean \
	&& set +x \
	&& cd "$(abs_top_builddir)" \
	&& { chmod u+w -R "$$DCDIR/" && rm -r "$$DCDIR/" ; } \
	&& echo "OK: archive ready for distribution: $(disttarball)" | sed '1h; 1s/./=/g; 1p; 1x; $$p; $$x'
distuninstallcheck:
	$Q test -n '$(distuninstallcheck_dir)' || { echo '$@: missing distuninstallcheck_dir' >&2; false; }
	$Q cd '$(distuninstallcheck_dir)' \
	  && test `$(distuninstallcheck_listfiles) | sed 's|^\./|$(prefix)/|' | wc -l` -eq 0 \
	  || { echo "$@: ERROR: files left after uninstall:" ; \
	       $(distuninstallcheck_listfiles) ; \
	       false; } >&2

# == distuninstallcheck ignores ==
# Some files remain after distcheck, that we cannot clean up. So we role our own listfiles filter.
distuninstallcheck_listfiles  = find . -type f $(patsubst .../%, ! -path \*/%, $(distuninstallcheck_ignores)) -print
#distuninstallcheck_listfiles = find . -type f -print	# original automake-1.14.1 setting
distuninstallcheck_ignores = $(strip	\
	.../share/mime/subclasses	.../share/mime/XMLnamespaces	.../share/mime/globs2	\
	.../share/mime/version		.../share/mime/icons		.../share/mime/types	\
	.../share/mime/treemagic	.../share/mime/aliases		.../share/mime/magic	\
	.../share/mime/mime.cache	.../share/mime/generic-icons	.../share/mime/globs	\
	.../share/applications/mimeinfo.cache	\
)

# == ChangeLog ==
CHANGELOG_RANGE = $(shell git cat-file -e ce584d04999a7fb9393e1cfedde2048ba73e8878 && \
		    echo ce584d04999a7fb9393e1cfedde2048ba73e8878..HEAD || echo HEAD)
$>/ChangeLog: $(GITCOMMITDEPS)					| $>/
	$(QGEN)
	$Q git log --pretty='^^%ad  %an 	# %h%n%n%B%n' --first-parent \
		--abbrev=11 --date=short $(CHANGELOG_RANGE)	 > $@.tmp	# Generate ChangeLog with ^^-prefixed records
	$Q sed 's/^/	/; s/^	^^// ; s/[[:space:]]\+$$// '    -i $@.tmp	# Tab-indent commit bodies, kill trailing whitespaces
	$Q sed '/^\s*$$/{ N; /^\s*\n\s*$$/D }'			-i $@.tmp	# Compress multiple newlines
	$Q mv $@.tmp $@
	$Q test -s $@ || { mv $@ $@.empty ; ls -al --full-time $@.empty ; exit 1 ; }
CLEANFILES += $>/ChangeLog
