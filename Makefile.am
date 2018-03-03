# BEAST & BSE

include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)

SUBDIRS		= po sfi bse plugins drivers pybeast beast-gtk launchers library tools docs data .

# == make setup ==
DEFS                    += -DBSE_CONVENIENCE
QWAVCHECK                = @echo '  WAVCHECK' $@;
AM_CPPFLAGS             += -I$(top_srcdir) -I$(top_builddir)
lib_LTLIBRARIES          =
bin_PROGRAMS             =
bin_SCRIPTS              =
#noinst_DATA             =
noinst_SCRIPTS           =
noinst_PROGRAMS          =
noinst_LTLIBRARIES	 =

# == project tools and paths ==
YAPPS                    = $(PYTHON) $(topdir)/yapps2_deb/yapps2.py
AIDACC                   = $(topdir)/aidacc/aidacc
SFIDL                    = $(top_builddir)/sfi/sfidl
SFIDL_INC                = --nostdinc -I$(top_srcdir) -I$(top_builddir)
BSEFEXTRACT		 = $(top_builddir)/tools/bsefextract
BSEFCOMPARE		 = $(top_builddir)/tools/bsefcompare

# == early rules ==
# Ideally, all directories would provide a Makefile.sub and be included here
# in the appropriate order. Since we're not there yet, *some* Makefile.sub
# rules need to be build before recursing into subdirs with Makefile.am, and
# other Makefile.sub rules need to be build *after* recursing into subdirs.
# We solve this by building ./ last, and using BUILT_SOURCES to force builds
# of files required before we recurse into subdirs with Makefile.am.
BUILT_SOURCES            =

# == res/ ==
# define res_resfiles_list via immediate assignment
include res/Makefile.sub

# == aidacc/ ==
include aidacc/Makefile.sub
BUILT_SOURCES	+= aidacc-all

# == aidacc/tests/ ==
include aidacc/tests/Makefile.sub

# == ebeast/ ==
include ebeast/Makefile.sub
include ebeast/v8bse/Makefile.sub

# == Demos/ ==
include Demos/Makefile.sub

# == Instruments/ ==
include Instruments/Makefile.sub

# == Samples/ ==
include Samples/Makefile.sub

# == doc/ install ==
projectdocsdir   = $(beastdocdir)/
projectdocs_DATA = HACKING README NEWS COPYING
.SECONDARY: HACKING.md README.md		# pacify make(1) when shipping files generated from markdown
MAINTAINERCLEANFILES += HACKING

# == Markdown rules ==
%: %.md
	$(AM_V_GEN)
	$(Q) rm -f $@		# work around read-only target
	$(Q) $(PANDOC) -S -f markdown_github+pandoc_title_block-hard_line_breaks -t plain $< -o $@
	$(Q) chmod a-w $@	# prevent accidental target file edits
%.html: %.md
	$(AM_V_GEN)
	$(Q) rm -f $@		# work around read-only target
	$(Q) $(PANDOC) -s --toc -N -S -f markdown_github+pandoc_title_block-hard_line_breaks -t html \
		--email-obfuscation=javascript $< -o $@
	$(Q) chmod a-w $@	# prevent accidental target file edits
MAINTAINERCLEANFILES += README README.html

# == buildtest ==
installcheck-buildtest:
	$(Q) cd . \
	&& echo '#include <bse/bse.hh>'					 > $@.cc \
	&& echo 'extern "C" int main (int argc, char *argv[]) {'	>> $@.cc \
	&& echo '  Bse::init_async (&argc, argv, "bse-app-test");'	>> $@.cc \
	&& echo '  return 0; }'						>> $@.cc \
		; X=$$? ; echo -n "Create  BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$(Q) $(CXX) -Werror $@.cc -c `PKG_CONFIG_PATH="$(DESTDIR)$(bselibdir)/pkgconfig:$(libdir)/pkgconfig:$$PKG_CONFIG_PATH" pkg-config --cflags bse` \
		; X=$$? ; echo -n "Compile BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$(Q) $(CXX) -Werror $@.o -o $@ `PKG_CONFIG_PATH="$(DESTDIR)$(bselibdir)/pkgconfig:$(libdir)/pkgconfig:$$PKG_CONFIG_PATH" pkg-config --libs bse` \
		; X=$$? ; echo -n "Link    BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$(Q) LD_LIBRARY_PATH="$(DESTDIR)$(bselibdir):$$LD_LIBRARY_PATH" ./$@ \
		; X=$$? ; echo -n "Execute BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$(Q) rm -f $@.o $@.cc $@
.PHONY: installcheck-buildtest
installcheck-local: installcheck-buildtest

# == topbuildid.cc ==
topbuildid ::= $(shell $(topdir)/misc/mkbuildid.sh -p topbuildid.cc topbuildid.hh) # "update rule" for topbuildid.cc
topbuildid.cc: # avoid explicit deps, 'topbuildid::=' ensures updates
CLEANFILES += topbuildid.cc	# preserve across 'mostlyclean' if unchanged to avoid expensive rebuilds
buildid:
	@echo $(topbuildid)

# == ChangeLog ==
CHANGELOG_RANGE = $(shell git cat-file -e ce584d04999a7fb9393e1cfedde2048ba73e8878 && \
		    echo ce584d04999a7fb9393e1cfedde2048ba73e8878..HEAD || echo HEAD)
ChangeLog: $(GITSTAMP)
	$(AM_V_GEN)
	$(Q) git log --pretty='^^%ad  %an 	# %h%n%n%B%n' --first-parent \
		--abbrev=11 --date=short $(CHANGELOG_RANGE)   > xgen-$(@F) # Generate ChangeLog with ^^-prefixed records
	$(Q) sed 's/^/	/; s/^	^^// ; s/[[:space:]]\+$$// '    -i xgen-$(@F) # Tab-indent commit bodies, kill trailing whitespaces
	$(Q) sed '/^\s*$$/{ N; /^\s*\n\s*$$/D }'                -i xgen-$(@F) # Compress multiple newlines
	$(Q) mv xgen-$(@F) $@
	$(Q) test -s $@ || { mv $@ $@.empty ; ls -al --full-time $@.empty ; exit 1 ; }
noinst_DATA          += ChangeLog
MAINTAINERCLEANFILES += ChangeLog
BUILT_SOURCES        += ChangeLog

# == dist ==
distname      = $(PACKAGE)-$(VERSION)
distnamexz    = $(distname).tar.xz
dist: all
	$(Q) test 0 = `git diff HEAD | wc -l` || echo -e "#\n# $@: WARNING: working tree unclean\n#" >&2
	$(Q) git archive --format=tar --prefix=$(distname)/ HEAD >$(distname).tar
	$(Q) rm -rf .tmpdist/ &&					\
	  mkdir .tmpdist/ &&						\
	  tar -C .tmpdist/ -xf $(distname).tar &&			\
	  $(MAKE) $(AM_MAKEFLAGS) extra-gitdist				\
		  EXTRA_GITDIST_DIR=`pwd`"/.tmpdist/$(distname)/" &&	\
	  cd .tmpdist/$(distname)/ &&					\
	  ./autogen.sh --help >/dev/null &&				\
	  cd ../ &&							\
	  tar -f ../$(distname).tar -hu $(distname)/ &&			\
	  cd .. &&							\
	  rm -fr .tmpdist/
	$(Q) rm -f $(distname).tar.xz && xz $(distname).tar && test -e $(distnamexz)
	$(Q) echo "Archive ready: $(distnamexz)" | sed '1h; 1s/./=/g; 1p; 1x; $$p; $$x'
.PHONY: dist
EXTRA_DIST += ChangeLog README

# == distcheck ==
# Distcheck aims:
# - use srcdir==builddir;
# - build *outside* the original source tree to catch missing files or dirs, and without picking up parent directory contents;
# - support parallel builds;
# - verify that no CLEANFILES are shipped in dist tarball;
# - check that $(DESTDIR) is properly honored in installation rules.
# distcheck_uniqdir - directory for build tests, outside of srcdir, unique per user and checkout
# distcheck_uniqdir = distcheck-$(shell printf %d-%04x\\n $$UID 0x`X=$$(pwd) && echo -n "$$X" | md5sum | sed 's/^\(....\).*/\1/'`)
distcheck_uniqdir = distcheck-$(shell python -c "import os, md5; print ('%u-%s' % (os.getuid(), md5.new (os.getcwd()).hexdigest()[:4]))")
distcheck: dist
	$(Q) TMPDIR="$${TMPDIR-$${TEMP-$${TMP-/tmp}}}" \
	&& DCDIR="$$TMPDIR/$(distcheck_uniqdir)" \
	&& TARBALL=$$(readlink -f "$(distnamexz)") \
	&& test -n "$$TMPDIR" -a -n "$(distcheck_uniqdir)" -a -n "$$DCDIR" -a -n "$(distname)" -a -n "$$TARBALL" \
	&& { test ! -e "$$DCDIR/" || { chmod u+w -R "$$DCDIR/" && rm -r "$$DCDIR/" ; } ; } \
	&& mkdir -p "$$DCDIR" \
	&& set -x \
	&& cd "$$DCDIR" \
	&& tar xf "$$TARBALL" \
	&& cd "$(distname)" \
	&& ./configure --prefix="$$DCDIR/inst" $(AM_DISTCHECK_CONFIGURE_FLAGS) $(DISTCHECK_CONFIGURE_FLAGS) \
	&& touch dc-buildtree-cleaned \
	&& find . -print >dc-buildtree-files \
	&& $(MAKE) $(AM_MAKEFLAGS) clean \
	&& find . -print >dc-buildtree-cleaned \
	&& diff -u dc-buildtree-files dc-buildtree-cleaned \
	&& $(MAKE) $(AM_MAKEFLAGS) -j`nproc` \
	&& $(MAKE) $(AM_MAKEFLAGS) check \
	&& $(MAKE) $(AM_MAKEFLAGS) install \
	&& $(MAKE) $(AM_MAKEFLAGS) installcheck \
	&& $(MAKE) $(AM_MAKEFLAGS) uninstall \
	&& $(MAKE) $(AM_MAKEFLAGS) distuninstallcheck-hook \
	&& $(MAKE) $(AM_MAKEFLAGS) distuninstallcheck distuninstallcheck_dir="$$DCDIR/inst" \
	&& chmod a-w -R "$$DCDIR/inst" \
	&& mkdir -m 0700 "$$DCDIR/destdir" \
	&& $(MAKE) $(AM_MAKEFLAGS) DESTDIR="$$DCDIR/destdir" install \
	&& $(MAKE) $(AM_MAKEFLAGS) DESTDIR="$$DCDIR/destdir" uninstall \
	&& $(MAKE) $(AM_MAKEFLAGS) DESTDIR="$$DCDIR/destdir" distuninstallcheck-hook \
	&& $(MAKE) $(AM_MAKEFLAGS) DESTDIR="$$DCDIR/destdir" distuninstallcheck distuninstallcheck_dir="$$DCDIR/destdir" \
	&& $(MAKE) $(AM_MAKEFLAGS) clean \
	&& set +x \
	&& cd "$(abs_top_builddir)" \
	&& { chmod u+w -R "$$DCDIR/" && rm -r "$$DCDIR/" ; } \
	&& echo "OK: archive ready for distribution: $$TARBALL" | sed '1h; 1s/./=/g; 1p; 1x; $$p; $$x'
distuninstallcheck:
	@test -n '$(distuninstallcheck_dir)' || { echo '$@: missing distuninstallcheck_dir' >&2; false; }
	@cd '$(distuninstallcheck_dir)' && \
	test `$(distuninstallcheck_listfiles) | sed 's|^\./|$(prefix)/|' | wc -l` -eq 0 \
	|| { echo "$@: ERROR: files left after uninstall:" ; \
	     $(distuninstallcheck_listfiles) ; \
	     false; } >&2
.PHONY: distcheck distuninstallcheck distuninstallcheck-hook

# == distcheck-checks ==
DISTCHECK_PO = 1
check-update-po:
	$(Q) [ "$(DISTCHECK_PO)" = "0" ] || $(MAKE) -C po check-update-po
DISTCHECK_HOOK = distcheck # help add dependencies to 'distcheck' without overriding its definition
$(DISTCHECK_HOOK): check-update-po

# == distuninstallcheck ignores ==
# Some files remain after distcheck, that we cannot clean up. So we role our own listfiles filter.
distuninstallcheck_listfiles  = find . -type f $(patsubst ./%, ! -path \*/%, $(uninstall_filter_files)) -print
#distuninstallcheck_listfiles = find . -type f -print	# original automake-1.14.1 setting
uninstall_filter_files = $(strip	\
	*/share/mime/subclasses		*/share/mime/XMLnamespaces	*/share/mime/globs2	\
	*/share/mime/version		*/share/mime/icons		*/share/mime/types	\
	*/share/mime/treemagic		*/share/mime/aliases		*/share/mime/magic	\
	*/share/mime/mime.cache		*/share/mime/generic-icons	*/share/mime/globs	\
	*/share/applications/mimeinfo.cache	\
)

# == cleanups ==
MOSTLYCLEANFILES +=		\
	distcheck\.log
# Use am__CONFIG_DISTCLEAN_FILES instead of MAINTAINERCLEANFILES, because the latter
# is cleared before recursing into subdirs, while the former is cleared later.
am__CONFIG_DISTCLEAN_FILES = 	\
	Makefile\.in		\
	config/compile		\
	aclocal\.m4		\
	autogen\.log		\
	config\.log		\
	configure		\
	configure\.h\.in	\
	configure\.h\.in~	\
	config/config\.guess	\
	config/config\.sub	\
	config/depcomp		\
	config/install-sh	\
	config/intltool\.m4	\
	config/libtool\.m4	\
	config/ltmain\.sh	\
	config/ltoptions\.m4	\
	config/ltsugar\.m4	\
	config/ltversion\.m4	\
	config/lt~obsolete\.m4	\
	config/missing		\
	config/py-compile	\
	config\.status		\
	config\.cache		\
	configure\.lineno	\
	config\.status\.lineno

# == tests/ ==
include tests/Makefile.sub

# == af-tests/ ==
include af-tests/Makefile.sub
