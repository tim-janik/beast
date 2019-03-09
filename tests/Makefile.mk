# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/tests/*.d)
CLEANDIRS += $(wildcard $>/tests/)
tests/rpath..libbse ::= ../lib

# == tests/ files ==
tests/suite1.sources		::= $(strip	\
	tests/blocktests.cc			\
	tests/filterdesign.cc			\
	tests/filtertest.cc			\
	tests/firhandle.cc			\
	tests/loophandle.cc			\
	tests/misctests.cc			\
	tests/resamplehandle.cc			\
	tests/subnormals-aux.cc			\
	tests/subnormals.cc			\
	tests/suite1-main.cc			\
	tests/suite1-randomhash.cc		\
	tests/testfft.cc			\
	tests/testresampler.cc			\
	tests/testresamplerq.cc			\
	tests/testwavechunk.cc			\
)

# == suite1 defs ==
tests/suite1			::= $>/tests/suite1
tests/suite1.objects		::= $(sort $(tests/suite1.sources:%.cc=$>/%.o))

# == subdirs ==
include tests/audio/Makefile.mk

# == suite1 rules ==
$(tests/suite1.objects):	$(bse/libbse.deps) | $>/tests/
$(tests/suite1.objects):	EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_TEST, \
	$(tests/suite1), \
	$(tests/suite1.objects), \
	$(lib/libbse.so), \
	-lbse-$(VERSION_MAJOR) $(GLIB_LIBS), \
	$(tests/rpath..libbse))

# == check-bse-loading ==
# This test checks that all .bse files contained in the beast tarball
# will load without any warnings or errors being issued. At first,
# we split the (long) list of files into multiple lists that can be
# checked in parallel. For each file in a list, unless it matches
# the skip-pattern, try to load it and check the output logs.
$>/tests/bsefiles.lst: FORCE # generate checklist-a .. checklist-e
	$(QGEN)
	$Q find . -type f -name '*.bse' > $@
	$Q split -n l/8 -a 1 $@ $@-
	$Q rm -f $@
# Check-load ensures BSE loading works, it needs all available samples and plugins
tests/check-load = $(strip						\
	$(tools/bsetool)						\
	  $(if $(findstring 1, $(V)),, --quiet)				\
	  check-load							\
	  --bse-pcm-driver null=nosleep					\
	  --bse-midi-driver null					\
	  --bse-override-plugin-globs '$>/plugins/*.so'			\
	  --bse-override-sample-path 'tests/audio:media/Samples'	\
	  --bse-rcfile /dev/null )
tests/skipload = 'DUMMY.bse'
$>/tests/bsefiles.lst-%-test: $>/tests/bsefiles.lst $(tools/bsetool)
	$(QECHO) CHECK $(@:-test=)
	$Q for tfile in `cat $(@:-test=)` ; do				\
	  if echo "$$tfile" | egrep -q $(tests/skipload) ; then		\
	    echo "  SKIP     Loading: $$tfile" ;			\
	  else								\
	    $(tests/check-load) "$$tfile" 2>&1 | tee $@.log ;		\
	    test ! -s $@.log || exit 1 &&				\
	    echo "  OK       Loading: $$tfile" ;			\
	  fi ; done ; rm -f $@.log $(@:-test=)
tests/check-bse-loading: \
				$>/tests/bsefiles.lst-a-test	\
				$>/tests/bsefiles.lst-b-test	\
				$>/tests/bsefiles.lst-c-test	\
				$>/tests/bsefiles.lst-d-test	\
				$>/tests/bsefiles.lst-e-test	\
				$>/tests/bsefiles.lst-f-test	\
				$>/tests/bsefiles.lst-g-test	\
				$>/tests/bsefiles.lst-h-test
CHECK_TARGETS += tests/check-bse-loading
