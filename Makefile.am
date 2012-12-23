# BEAST/BSE - Better Audio System / Better Sound Engine
include $(top_srcdir)/Makefile.decl

SUBDIRS = . data birnet sfi bse plugins drivers shell beast-gtk launchers library tools po tests docs

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

# --- release helpers ---
release-news:
	@$(srcdir)/mkrelease.sh news
release-check:
	@echo "Checking for unlisted contributors..."
	@$(srcdir)/mkrelease.sh contributors -X -B timj,stw -C $(srcdir)/beast-gtk/bstmain.c
release-upload: ChangeLog
	$(srcdir)/mkrelease.sh upload -U dist.testbit.eu:dist/beast/v0.7/ -E configure.in:BSE_MICRO_VERSION
.PHONY: release-news release-check release-upload

# == Dist Fixes ==
# Some files remain after distcheck, that we cannot clean up. So we role our own listfiles filter.
distuninstallcheck_listfiles  = find . -type f $(patsubst ./%, ! -path \*/%, $(uninstall_filter_files)) -print
#distuninstallcheck_listfiles = find . -type f -print	# original automake-1.11 setting
uninstall_filter_files = $(strip	\
	./share/mime/subclasses		./share/mime/XMLnamespaces	./share/mime/globs2	\
	./share/mime/version		./share/mime/icons		./share/mime/types	\
	./share/mime/treemagic		./share/mime/aliases		./share/mime/magic	\
	./share/mime/mime.cache		./share/mime/generic-icons	./share/mime/globs	\
)

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
	|| ( $(MAKE) report && mv ./report.out /tmp/beast-distcheck$$$$.report )
.PHONY: costly-distcheck-tests
