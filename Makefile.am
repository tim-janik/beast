# BEAST/BSE - Bedevilled Audio System / Bedevilled Sound Engine
# Copyright (C) 1998-2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . data birnet sfi bse plugins shell beast-gtk launchers library tools po tests doxer docs web

noinst_DATA =

# require automake 1.9
AUTOMAKE_OPTIONS = 1.9 dist-bzip2 no-dist-gzip -Wno-portability
ACLOCAL_AMFLAGS = -I autotools

site site-preview site-update site-update-all site-update-pages site-update-galleries:
	@$(MAKE) -C web/ $(AM_MAKEFLAGS) $@

# extra dependencies
configure: birnet/acbirnet.m4 birnet/configure.inc

CLEANFILES += $(strip		\
)

EXTRA_DIST += $(strip		\
	TODO			\
	COPYING.GPL		\
	COPYING.LGPL		\
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

# --- release-upload ---
release-upload: ChangeLog
	$(srcdir)/mkrelease.sh upload -U localhost:tmp -E configure.in:BSE_MICRO_VERSION
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

# run 'make report' after installcheck from distcheck
DISTCHECK_CONFIGURE_FLAGS = --enable-distcheck-tests
INSTALLCHECK_HOOK = installcheck # use indirection to emulate installcheck-hook
$(INSTALLCHECK_HOOK): costly-distcheck-tests
costly-distcheck-tests:
	@case "$(BEAST_BUILD)" in *:distcheck-tests:*) $(MAKE) report ;; esac
.PHONY: costly-distcheck-tests

# remove non-distributed po files from distributed POTFILES.in
dist-hook:
	sed 's,^\(drivers/\),# \1,' < $(distdir)/po/POTFILES.in > $(distdir)/po/POTFILES.tmp
	mv -f $(distdir)/po/POTFILES.tmp $(distdir)/po/POTFILES.in
