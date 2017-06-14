# BEAST & BSE

include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)

SUBDIRS		= . sfi bse plugins drivers shell pybeast beast-gtk launchers library tools ebeast po docs data tests

# == make setup ==
noinst_DATA	=

# == doc/ install ==
projectdocsdir   = $(beastdocdir)/
projectdocs_DATA = HACKING README NEWS COPYING
EXTRA_DIST      += $(projectdocs_DATA)
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


# check: message
ALL_CHECKED_MSG = "All package tests passed"
check: check-recursive
	@echo $(ALL_CHECKED_MSG) | sed 's/./=/g'
	@echo $(ALL_CHECKED_MSG)
	@echo $(ALL_CHECKED_MSG) | sed 's/./=/g'

# == BSE App Test ==
bse-app-test:
	$(Q) cd . \
	&& echo '#include <bse/bse.hh>'					 > tmpx.cc \
	&& echo 'extern "C" int main (int argc, char *argv[]) {'	>> tmpx.cc \
	&& echo '  Bse::init_async (&argc, argv, "bse-app-test");'	>> tmpx.cc \
	&& echo '  return 0; }'						>> tmpx.cc \
		; X=$$? ; echo -n "Create  BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$(Q) $(CXX) -Werror tmpx.cc -c `PKG_CONFIG_PATH="$(bselibdir)/pkgconfig:$(libdir)/pkgconfig:$$PKG_CONFIG_PATH" pkg-config --cflags bse` \
		; X=$$? ; echo -n "Compile BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$(Q) $(CXX) -Werror tmpx.o -o tmpx `PKG_CONFIG_PATH="$(bselibdir)/pkgconfig:$(libdir)/pkgconfig:$$PKG_CONFIG_PATH" pkg-config --libs bse` \
		; X=$$? ; echo -n "Link    BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$(Q) LD_LIBRARY_PATH="$(bselibdir):$$LD_LIBRARY_PATH" ./tmpx \
		; X=$$? ; echo -n "Execute BSE sample program: " ; test 0 == $$X && echo OK || { echo FAIL; exit $$X ; }
	$(Q) rm -f tmpx.o tmpx.cc tmpx
installcheck-local: bse-app-test


# == Dist Fixes ==
# Some files remain after distcheck, that we cannot clean up. So we role our own listfiles filter.
distuninstallcheck_listfiles  = find . -type f $(patsubst ./%, ! -path \*/%, $(uninstall_filter_files)) -print
#distuninstallcheck_listfiles = find . -type f -print	# original automake-1.14.1 setting
uninstall_filter_files = $(strip	\
	./share/mime/subclasses		./share/mime/XMLnamespaces	./share/mime/globs2	\
	./share/mime/version		./share/mime/icons		./share/mime/types	\
	./share/mime/treemagic		./share/mime/aliases		./share/mime/magic	\
	./share/mime/mime.cache		./share/mime/generic-icons	./share/mime/globs	\
)

# == topbuildid.cc ==
EXTRA_DIST  += misc/mkbuildid.sh topbuildid.hh
topbuildid ::= $(shell $(topdir)/misc/mkbuildid.sh -p topbuildid.cc topbuildid.hh) # "update rule" for topbuildid.cc
topbuildid.cc: # avoid explicit deps, 'topbuildid::=' ensures updates
CLEANFILES += topbuildid.cc	# preserve across 'mostlyclean' if unchanged to avoid expensive rebuilds
buildid:
	@echo $(topbuildid)

# == ChangeLog & Release Rules ==
CHANGELOG_RANGE = $(shell git cat-file -e ce584d04999a7fb9393e1cfedde2048ba73e8878 && \
		    echo ce584d04999a7fb9393e1cfedde2048ba73e8878..HEAD || echo HEAD)
ChangeLog: $(GITSTAMPS)
	$(AM_V_GEN)
	$(Q) git log --pretty='^^%ad  %an 	# %h%n%n%B%n' --abbrev=11 \
		--date=short --first-parent $(CHANGELOG_RANGE)   > xgen-$(@F) # Generate ChangeLog with ^^-prefixed records
	$(Q) sed 's/^/	/; s/^	^^// ; s/[[:space:]]\+$$// '    -i xgen-$(@F) # Tab-indent commit bodies, kill trailing whitespaces
	$(Q) sed '/^\s*$$/{ N; /^\s*\n\s*$$/D }'                -i xgen-$(@F) # Compress multiple newlines
	$(Q) mv xgen-$(@F) $@
	$(Q) test -s $@ || { mv $@ $@.empty ; ls -al --full-time $@.empty ; exit 1 ; }
noinst_DATA          += ChangeLog
MAINTAINERCLEANFILES += ChangeLog
EXTRA_DIST           += ChangeLog taptool.sh

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
	&& TARBALL=`readlink -f $(firstword $(DIST_ARCHIVES))` \
	&& test -n "$$TMPDIR" -a -n "$(distcheck_uniqdir)" -a -n "$$DCDIR" -a -n "$(distdir)" -a -n "$$TARBALL" \
	&& { test ! -e "$$DCDIR/" || { chmod u+w -R "$$DCDIR/" && rm -r "$$DCDIR/" ; } ; } \
	&& mkdir -p "$$DCDIR" \
	&& set -x \
	&& cd "$$DCDIR" \
	&& tar xf "$$TARBALL" \
	&& cd "$(distdir)" \
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
.PHONY: distcheck distuninstallcheck-hook

# == distcheck-checks ==
DISTCHECK_PO = 1
check-update-po:
	$(Q) [ "$(DISTCHECK_PO)" = "0" ] || $(MAKE) -C po check-update-po
DISTCHECK_HOOK = distcheck # help add dependencies to 'distcheck' without overriding its definition
$(DISTCHECK_HOOK): check-update-po
