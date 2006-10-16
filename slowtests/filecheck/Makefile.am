# beast/tests/filecheck
# Copyright (C) 2003-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)
DEFS += -DG_LOG_DOMAIN="\"BEAST-TEST\"" # -DG_DISABLE_CONST_RETURNS
SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)


# === BSE file check ===
# this test checks that all .bse files contained in the beast tarball
# will load without any warnings or errors being issued. this is done
# by first constructing the tarball file list via the distfile-list:-rule
# which is specified in Makefile.decl and included in each subdir, then
# filtering out bse files and finally trying to load these without warnings.
DISTFILE_LIST_NAME=$(shell pwd)/distfiles.list
distfiles.list: Makefile
	rm -f distfiles.list
	test ! -e distfiles.list
	test ! -e $(DISTFILE_LIST_NAME)
	echo > $(DISTFILE_LIST_NAME)
	test -e distfiles.list
	test -e $(DISTFILE_LIST_NAME)
	cd $(top_builddir) && make distfile-list DISTFILE_LIST=$(DISTFILE_LIST_NAME)
.PHONY: bse-loading-check bse-version-check
bse-version-check: distfiles.list
	TESTVERSION="$(BSE_MAJOR_VERSION).$(BSE_MINOR_VERSION)." ;	\
	for tfile in `grep '\.bse$$' distfiles.list` ; do		\
	  file="$(top_srcdir)/$$tfile" ;				\
	  echo "Check project version: $$file" ;			\
	  head -n5 "$$file" |						\
	    grep -Fq "(bse-version \"$$TESTVERSION" ||			\
	      { grep -F bse-version "$$file" ; exit 1 ; }		\
	done
	@MESSAGETEXT="All tested BSE files passed version check"	\
	&& echo $$MESSAGETEXT | sed 's/./=/g' 				\
	&& echo $$MESSAGETEXT		 				\
	&& echo $$MESSAGETEXT | sed 's/./=/g'
SKIP_PATTERN = "tests/latency/midi-latency.bse|tests/audio/minisong.bse"
bse-loading-check: distfiles.list
	for tfile in `grep '\.bse$$' distfiles.list` ; do		\
	  file="$(top_srcdir)/$$tfile" ;				\
	  if echo "$$tfile" | egrep -q $(SKIP_PATTERN) ; then		\
	    echo "-Skipping project: $$file" ;				\
	  else								\
	    echo "Test load project: $$file" ;				\
	    $(BSESCM) "$$file" 2>&1 |					\
	      tee warnings.tmp ;					\
	    test ! -s "warnings.tmp" || exit 1 ;			\
	  fi ;								\
	done
	@MESSAGETEXT="All tested BSE files passed loading test"		\
	&& echo $$MESSAGETEXT | sed 's/./=/g' 				\
	&& echo $$MESSAGETEXT		 				\
	&& echo $$MESSAGETEXT | sed 's/./=/g'
CLEANFILES += distfiles.list warnings.tmp
check-after: bse-version-check bse-loading-check
plugin_globs = '$(top_builddir)/plugins/.libs/*.so:$(top_builddir)/plugins/freeverb/.libs/*.so'
EXTRA_DIST += checkproject.scm
BSESCM = $(strip								\
	$(top_builddir)/shell/bsescm-$(BIN_VERSION)				\
	  -p null -m null							\
	  --bse-rcfile "/dev/null" --bse-no-load				\
	  --bse-override-plugin-globs '$(plugin_globs)'				\
	  -s "$(top_srcdir)/tests/filecheck/checkproject.scm"			\
)
