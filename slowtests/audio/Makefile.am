# Beast Tests
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl


# === feature test tools ===
BSE2WAV = $(strip							\
	$(top_builddir)/shell/bsesh-$(BIN_VERSION)			\
	  --bse-mixing-freq=48000 -p null -m null			\
	  --bse-rcfile "/dev/null" --bse-no-load			\
	  --bse-override-plugin-globs $(builddirplugins)		\
	  -s bsetowav.scm 						\
)
builddirplugins = '$(top_builddir)/plugins/.libs/*.so:$(top_builddir)/plugins/freeverb/.libs/*.so'
BSEFEXTRACT = $(top_builddir)/tools/bsefextract
BSEFCOMPARE = $(top_builddir)/tools/bsefcompare
EXTRA_DIST += bsetowav.scm

# === run tests upon make check ===
FEATURE_TESTS = 
check-am: check-feature-tests # run test rules *before* ordinary tests
# check-local: check-feature-tests # run test rules *after* ordinary tests
.PHONY: check-feature-tests $(FEATURE_TESTS)
FEATURE_TESTS_MSG = "All $(words $(FEATURE_TESTS)) audio tests passed"
check-feature-tests: # $(FEATURE_TESTS)
	@for tst in $(FEATURE_TESTS) ; do 			\
	  echo "TEST: $$tst" ;					\
	  $(MAKE) -C ./ $(AM_MAKEFLAGS) $$tst || exit $? ;	\
	  echo "PASS: $$tst" ;					\
	done
	@echo $(FEATURE_TESTS_MSG) | sed 's/./=/g'
	@echo $(FEATURE_TESTS_MSG)
	@echo $(FEATURE_TESTS_MSG) | sed 's/./=/g'

# === feature test temporaries ===
CLEANFILES += *.tmp *.wav

# === feature tests ===
FEATURE_TESTS += bseadder-test
EXTRA_DIST += bseadder.bse bseadder.ref
bseadder-test:
	$(BSE2WAV) bseadder.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) bseadder.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += balance-test
EXTRA_DIST += balance.bse balance.ref
balance-test:
	$(BSE2WAV) balance.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(BSEFCOMPARE) balance.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += minisong-test
EXTRA_DIST += minisong.bse frowzy-drums.bsewave minisong.ref
minisong-test:
	$(BSE2WAV) minisong.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(BSEFCOMPARE) minisong.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += syndrum-test
EXTRA_DIST += syndrum.bse syndrum.ref
syndrum-test:
	$(BSE2WAV) syndrum.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(BSEFCOMPARE) syndrum.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += velocity-test
EXTRA_DIST += velocity.bse velocity.ref
velocity-test:
	$(BSE2WAV) velocity.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(BSEFCOMPARE) velocity.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav
