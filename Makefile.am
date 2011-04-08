# BEAST/BSE - Better Audio System / Better Sound Engine
# Copyright (C) 1998-2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . data birnet sfi bse plugins drivers shell beast-gtk launchers library tools po tests doxer docs

noinst_DATA =

# require automake 1.9
AUTOMAKE_OPTIONS = 1.9 dist-bzip2 no-dist-gzip -Wno-portability
ACLOCAL_AMFLAGS = -I autotools

site site-preview site-update site-update-all site-update-pages site-update-galleries:
	@$(MAKE) -C web/ $(AM_MAKEFLAGS) $@

# extra dependencies
configure: birnet/acbirnet.m4 birnet/configure.inc

EXTRA_DIST += $(strip		\
	TODO			\
	topconfig.h		\
)

# check: message
ALL_CHECKED_MSG = "All package tests passed"
check: check-recursive
	@echo $(ALL_CHECKED_MSG) | sed 's/./=/g'
	@echo $(ALL_CHECKED_MSG)
	@echo $(ALL_CHECKED_MSG) | sed 's/./=/g'

# --- ChangeLog ---
ChangeLog:	$(shell $(srcdir)/mkrelease.sh commit-stamps)
	$(srcdir)/mkrelease.sh ChangeLog \
	  -R ce584d04999a7fb9393e1cfedde2048ba73e8878..HEAD
noinst_DATA += ChangeLog
EXTRA_DIST  += ChangeLog mkrelease.sh

# --- release-news ---
release-news:
	$(srcdir)/mkrelease.sh news
.PHONY: release-news

# --- release-contributors ---
release-contributors:
	$(srcdir)/mkrelease.sh contributors -B timj,stw -C $(srcdir)/beast-gtk/bstmain.c
.PHONY: release-contributors

# --- release-upload ---
release-upload: ChangeLog
	$(srcdir)/mkrelease.sh upload -U beast:dist/beast/ -E configure.in:BSE_MICRO_VERSION
.PHONY: release-upload

# --- dist fixes ---
uninstall_filter = $(strip	\
	./share/mime/subclasses		./share/mime/icons	./share/mime/generic-icons \
	./share/mime/globs		./share/mime/types	./share/mime/XMLnamespaces \
	./share/mime/mime.cache		./share/mime/magic	./share/mime/globs2 \
	./share/mime/treemagic		./share/mime/aliases	\
)
#distuninstallcheck_listfiles = find . -type f -print	# original automake-1.11 setting
distuninstallcheck_listfiles  = find . -type f $(patsubst ./%, ! -path \*/%, $(uninstall_filter)) -print
clean_filter = $(strip		\
	./report.out		\
)
#distcleancheck_listfiles     = find . -type f -print	# original automake-1.11 setting
distcleancheck_listfiles      = find . -type f $(patsubst ./%, ! -path \*/%, $(clean_filter)) -print

# (test_option, INPUTSTRING, YESOPT, [NOOPT]) :- true if YESOPT is present
test_option = test `echo ":$(strip $(1)):" | sed 's/.*:$(strip $(2))://g' | wc -c` -lt \
      `echo ":$(strip $(1)):" | sed 's/.*:$(or $(strip $(3)), no-$(strip $(2)))://g' | wc -c`

# by default, run 'make report' after installcheck from inside distcheck
DISTCHECK_CONFIGURE_FLAGS = --enable-distcheck-tests
INSTALLCHECK_HOOK = installcheck # use indirection to emulate installcheck-hook
$(INSTALLCHECK_HOOK): costly-distcheck-tests
# --enable-distcheck-tests causes BEAST_BUILD_DEFAULTS=distcheck-tests which can
# be overridden by the user with BEAST_BUILD=no-distcheck-tests
costly-distcheck-tests:
	@! $(call test_option, $(BEAST_BUILD_DEFAULTS):$(BEAST_BUILD), distcheck-tests) \
	|| $(MAKE) report
.PHONY: costly-distcheck-tests

# remove non-distributed po files from distributed po/POTSCAN
dist-hook:
	sed 's,^\(drivers/\),# \1,' < $(distdir)/po/POTSCAN > $(distdir)/po/POTSCAN.tmp
	mv -f $(distdir)/po/POTSCAN.tmp $(distdir)/po/POTSCAN
