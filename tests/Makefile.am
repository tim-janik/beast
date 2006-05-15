# beast/tests
# Copyright (C) 2003 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = audio latency scripts bse

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)
DEFS += -DG_LOG_DOMAIN="\"BEAST-TEST\"" # -DG_DISABLE_CONST_RETURNS
SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)

# === .bse file check ===
# this test checks that all .bse files contained in the beast tarball
# will load without any warnings or errors being issued. this is done
# by first constructing a file list via the distfile-list:-rule which
# is specified in Makefile.decl, then filtering out bse files and then
# trying to load these.
DISTFILE_LIST_NAME=$(shell pwd)/distfiles.list
distfiles.list: Makefile
	rm -f distfiles.list
	test ! -e distfiles.list
	test ! -e $(DISTFILE_LIST_NAME)
	echo > $(DISTFILE_LIST_NAME)
	test -e distfiles.list
	test -e $(DISTFILE_LIST_NAME)
	cd $(top_srcdir) && make distfile-list DISTFILE_LIST=$(DISTFILE_LIST_NAME)
.PHONY: bse-check
bse-check: distfiles.list
	cd $(top_srcdir) &&				\
	for file in `grep '.bse$$' $(subdir)/distfiles.list` ; do	\
	  $(TOPDIR_BSESHELL) "$$file" | tee warnings.tmp ;	\
	  test -z "`cat warnings.tmp`" || exit 1 ;	\
	done
TOPDIR_BSESHELL = $(strip						\
	./shell/bsesh-$(BIN_VERSION)					\
	  -p null -m null						\
	  --bse-rcfile "/dev/null" --bse-no-load			\
	  --bse-override-plugin-globs './plugins/.libs/*.so'		\
)	# FIXME: this still uses the normal loading mechanism that also loads LADSPA

