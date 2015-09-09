# beast/tests/filecheck
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN="\"BEAST-TEST\"" # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(SFI_CPPFLAGS)

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

.PHONY: bse-version-check
bse-version-check: distfiles.list
	TESTVERSION="$(BST_MAJOR_VERSION).$(BST_MINOR_VERSION)." ;	\
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
check: # bse-version-check

.PHONY: bse-loading-check
SKIP_PATTERN = "tests/latency/midi-latency.bse"
bse-loading-check: distfiles.list
	@rm -f warnings.tmp
	for tfile in `grep '\.bse$$' distfiles.list` ; do		\
	  file="$(top_srcdir)/$$tfile" ;				\
	  if echo "$$tfile" | egrep -q $(SKIP_PATTERN) ; then		\
	    echo "-Skipping project: $$file" ;				\
	  else								\
	    echo "Test load project: $$file" ;				\
	    $(CHECK_LOAD) "$$file" 2>&1 |				\
	      tee warnings.tmp ;					\
	    test ! -s "warnings.tmp" || exit 1 ;			\
	  fi ;								\
	done
	@rm -f warnings.tmp
	@MESSAGETEXT="All tested BSE files passed loading test"		\
	&& echo $$MESSAGETEXT | sed 's/./=/g' 				\
	&& echo $$MESSAGETEXT		 				\
	&& echo $$MESSAGETEXT | sed 's/./=/g'
slowcheck: bse-loading-check

CLEANFILES += distfiles.list warnings.tmp
plugin_globs = '$(top_builddir)/plugins/.libs/*.so:$(top_builddir)/plugins/freeverb/.libs/*.so'
samplepath = '$(top_srcdir)/tests/audio:$(top_srcdir)/library/samples'
CHECK_LOAD = $(strip								\
	$(top_builddir)/bse/bsetool						\
	  $(if $(findstring 1, $(V)),, --quiet)					\
	  check-load								\
	  --bse-pcm-driver null=nosleep						\
	  --bse-midi-driver null						\
	  --bse-rcfile "/dev/null"						\
	  --bse-override-plugin-globs '$(plugin_globs)'				\
          --bse-override-sample-path $(samplepath)                              \
)
