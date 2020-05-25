# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/tests/*.d)
tests/cleandirs ::= $(wildcard $>/tests/)
CLEANDIRS        += $(tests/cleandirs)
ALL_TARGETS      += tests/all
tests/rpath..libbse ::= ../lib

# == tests/ files ==
tests/suite.sources		::= $(strip	\
	tests/basics.cc				\
	tests/aida-types.cc			\
	tests/benchmarks.cc			\
	tests/blocktests.cc			\
	tests/checkserialize.cc			\
	tests/explore-tests.cc			\
	tests/filterdesign.cc			\
	tests/filtertest.cc			\
	tests/firhandle.cc			\
	tests/ipc.cc				\
	tests/loophandle.cc			\
	tests/misctests.cc			\
	tests/resamplehandle.cc			\
	tests/subnormals-aux.cc			\
	tests/subnormals.cc			\
	tests/suite-main.cc			\
	tests/suite-randomhash.cc		\
	tests/testfft.cc			\
	tests/testresampler.cc			\
	tests/testresamplerq.cc			\
	tests/testwavechunk.cc			\
)

# == suite defs ==
tests/suite			::= $>/tests/suite
tests/suite.objects		::= $(call BUILDDIR_O, $(tests/suite.sources))

# == explore.idl ==
tests/explore.idl.outputs	::= $>/tests/explore_interfaces.hh $>/tests/explore_interfaces.cc

# == subdirs ==
include tests/audio/Makefile.mk

# == suite rules ==
$(tests/suite.objects):	$(bse/libbse.deps) | $>/tests/
$(tests/suite.objects):	EXTRA_INCLUDES ::= -I$> -Iexternal -I$>/tests $(GLIB_CFLAGS)
$(call BUILD_PROGRAM, \
	$(tests/suite), \
	$(tests/suite.objects), \
	$(lib/libbse.so), \
	-lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS), \
	$(tests/rpath..libbse))
tests/all: $(tests/suite)

# == explore.idl rules ==
$(tests/suite.objects):	$(tests/explore.idl.outputs)
$(call MULTIOUTPUT, $(tests/explore.idl.outputs)): tests/explore.idl $(aidacc/aidacc)	| $>/tests/
	$(QECHO) GEN $(tests/explore.idl.outputs) # aidacc generates %_interfaces.{hh|cc} from %.idl, and the real MULTIOUTPUT target name looks wierd
	$Q $(aidacc/aidacc) -x CxxStub -G strip-path=$(abspath .)/ -o $>/tests/ $<
	$Q sed -e '1i#define _(x) x' -i $>/tests/explore_interfaces.cc

# == check-loading ==
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
	  --bse-pcm-driver null						\
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
check-loading: \
				$>/tests/bsefiles.lst-a-test	\
				$>/tests/bsefiles.lst-b-test	\
				$>/tests/bsefiles.lst-c-test	\
				$>/tests/bsefiles.lst-d-test	\
				$>/tests/bsefiles.lst-e-test	\
				$>/tests/bsefiles.lst-f-test	\
				$>/tests/bsefiles.lst-g-test	\
				$>/tests/bsefiles.lst-h-test
CHECK_TARGETS += check-loading

# == run unit tests ==
check-suite: $(tests/suite) FORCE
	$(QGEN)
	$Q $(tests/suite) $(if $(PARALLEL_MAKE), -j )
CHECK_TARGETS += check-suite

# == bse-check-assertions ==
tests/suite-nobt ::= BSE_DEBUG=no-backtrace $(tests/suite)
check-assertions: $(tests/suite) FORCE
	$(QGEN)
	$Q $(tests/suite-nobt) --return_unless1 || $(QDIE) --return_unless1 failed
	$Q $(tests/suite-nobt) --assert_return1 || $(QDIE) --assert_return1 failed
	$Q (trap ':' SIGTRAP && $(tests/suite-nobt) --return_unless0) $(QSTDERR) ; test "$$?" -eq 7 || $(QDIE) --return_unless0 failed
	$Q (trap ':' SIGTRAP && $(tests/suite-nobt) --assert_return0) $(QSTDERR) ; test "$$?"  != 0 || $(QDIE) --assert_return0 failed
	$Q (trap ':' SIGTRAP && $(tests/suite-nobt) --assert_return_unreached) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --assert_return_unreached failed
	$Q (trap ':' SIGTRAP && $(tests/suite-nobt) --fatal_error) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --fatal_error failed
	@echo "  PASS    " $@
CHECK_TARGETS += check-assertions

# == AIDA benchmark ==
tests/aida-benchmark: $(tests/suite) FORCE
	$(QGEN)
	$Q $(tests/suite) --aida-bench
check-bench: tests/aida-benchmark

# == run unit tests ==
tests/suite-benchmark: $(tests/suite) FORCE
	$(QGEN)
	$Q $(tests/suite) --bench $(if $(PARALLEL_MAKE), -j )
check-bench: tests/suite-benchmark

# == tests/clean ==
tests/clean: FORCE
	rm -f -r $(tests/cleandirs)
