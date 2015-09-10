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
CLEANFILES += distfiles.list

SKIP_PATTERN = "tests/latency/midi-latency.bse"
checklist-%-test: checklists
	$(Q) $(if $(findstring 1, $(V)), set -x, :) ;		\
	  for tfile in `cat $(@F:-test=)` ; do			\
	    file="$(top_srcdir)/$$tfile" ;			\
	    if echo "$$tfile" | egrep -q $(SKIP_PATTERN) ; then	\
	      echo "SKIP - Loading: $$tfile" ;			\
	    else						\
	      $(CHECK_LOAD) "$$file" 2>&1 | tee $(@F).log ;	\
	      test ! -s $(@F).log || exit 1			\
	      && echo "OK   - Loading: $$tfile" ;		\
	    fi ; done ; rm -f $(@F).log $(@F:-test=)
checklists: distfiles.list # generate checklist-a .. checklist-e
	$(Q) grep '\.bse$$' distfiles.list > checklist
	$(Q) split -n l/5 -a 1 checklist checklist-
	$(Q) rm -f checklist
CLEANFILES += checklists checklist-? checklist-?-test checklist-?-test.log
check-local: checklist-a-test checklist-b-test checklist-c-test checklist-d-test checklist-e-test
plugin_globs = '$(top_builddir)/plugins/.libs/*.so:$(top_builddir)/plugins/freeverb/.libs/*.so'
samplepath   = '$(top_srcdir)/tests/audio:$(top_srcdir)/library/samples'
CHECK_LOAD   = $(strip	$(top_builddir)/bse/bsetool				\
			  $(if $(findstring 1, $(V)),, --quiet)			\
			  check-load						\
			  --bse-pcm-driver null=nosleep				\
			  --bse-midi-driver null				\
			  --bse-rcfile "/dev/null"				\
			  --bse-override-plugin-globs '$(plugin_globs)'		\
		          --bse-override-sample-path $(samplepath)		\
)

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

