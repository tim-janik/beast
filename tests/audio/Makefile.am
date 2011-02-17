# Beast Tests
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

# === feature test tools ===
builddirplugins = '$(top_builddir)/plugins/.libs/*.so:$(top_builddir)/plugins/freeverb/.libs/*.so'
samplepath = '$(top_srcdir)/tests/audio:$(top_srcdir)/library/samples'
BSEFEXTRACT = $(top_builddir)/tools/bsefextract
BSEFCOMPARE = $(top_builddir)/tools/bsefcompare
BSE2WAV = $(strip							\
	$(top_builddir)/shell/bsescm-$(BIN_VERSION)			\
	  --bse-mixing-freq=48000 -p null=nosleep -m null		\
	  --bse-rcfile "/dev/null" --bse-no-load			\
	  --bse-override-plugin-globs $(builddirplugins)		\
          --bse-override-sample-path $(samplepath)                      \
	  --bse-disable-randomization					\
	  -s $(srcdir)/bse2wav.scm					\
)
EXTRA_DIST += bse2wav.scm

# === audio feature test rule ===
FEATURE_TESTS = 
.PHONY: check-feature-tests $(FEATURE_TESTS)
check-feature-tests: # $(FEATURE_TESTS)
	@for tst in $(FEATURE_TESTS) ; do 			\
	  echo "TEST: $$tst" ;					\
	  $(MAKE) -C ./ $(AM_MAKEFLAGS) $$tst || exit $? ;	\
	  echo "PASS: $$tst" ;					\
	done
	@MESSAGETEXT="All $(words $(FEATURE_TESTS)) audio tests passed"	\
	&& echo $$MESSAGETEXT | sed 's/./=/g'				\
	&& echo $$MESSAGETEXT						\
	&& echo $$MESSAGETEXT | sed 's/./=/g'
slowcheck: check-feature-tests
# feature test temporaries
CLEANFILES += *.tmp *.wav

# === feature tests ===
FEATURE_TESTS += bseadder-test
EXTRA_DIST += bseadder.bse bseadder.ref
bseadder-test:
	$(BSE2WAV) $(srcdir)/bseadder.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/bseadder.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# Test the audio-diff characteristics of BseSummation     
FEATURE_TESTS += sum-diff-test
EXTRA_DIST += sum-diff-test.bse sum-diff-test.ref
sum-diff-test:
	$(BSE2WAV) $(srcdir)/sum-diff-test.bse $(@F).wav --seconds 25
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/sum-diff-test.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += balance-test
EXTRA_DIST += balance.bse balance.ref
balance-test:
	$(BSE2WAV) $(srcdir)/balance.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/balance.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += minisong-test
EXTRA_DIST += minisong.bse frowzy-drums.bsewave minisong.ref
minisong-test:
	$(BSE2WAV) $(srcdir)/minisong.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/minisong.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += syndrum-test
EXTRA_DIST += syndrum.bse syndrum.ref
syndrum-test:
	$(BSE2WAV) $(srcdir)/syndrum.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/syndrum.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += velocity-test
EXTRA_DIST += velocity.bse velocity.ref
velocity-test:
	$(BSE2WAV) $(srcdir)/velocity.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/velocity.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# DavOrgan module is a mono module, so we test only channel 0 (left)
FEATURE_TESTS += organsong-test
EXTRA_DIST += organsong.bse organsong.ref
organsong-test:
	$(BSE2WAV) $(srcdir)/organsong.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/organsong.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# ADSR Test checks the mono channel envelope rendering
FEATURE_TESTS += adsrtest
EXTRA_DIST += adsrtest.bse adsrtest.ref
adsrtest:
	$(BSE2WAV) $(srcdir)/adsrtest.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/adsrtest.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# checks that oscillator transposing works if the frequency is constant
FEATURE_TESTS += osctranspose1-test
EXTRA_DIST += osctranspose1.bse osctranspose1.ref
osctranspose1-test:
	$(BSE2WAV) $(srcdir)/osctranspose1.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/osctranspose1.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# checks that oscillator transposing works if the frequency is a signal
FEATURE_TESTS += osctranspose2-test
EXTRA_DIST += osctranspose2.bse osctranspose2.ref
osctranspose2-test:
	$(BSE2WAV) $(srcdir)/osctranspose2.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/osctranspose2.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += xtalstringssong-test
EXTRA_DIST += xtalstringssong.bse xtalstringssong.ref
xtalstringssong-test:
	$(BSE2WAV) $(srcdir)/xtalstringssong.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/xtalstringssong.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# a loop (from toplevel test/ directory)
# FEATURE_TESTS += freak-noise-test
EXTRA_DIST += freak-noise.bse freak-noise.ref
freak-noise-test:
	$(BSE2WAV) $(srcdir)/freak-noise.bse $(@F).wav --seconds 5
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/freak-noise.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# another simple loop (from toplevel test/ directory)
# FEATURE_TESTS += simple-loop-test
EXTRA_DIST += simple-loop.bse simple-loop.ref
simple-loop-test:
	$(BSE2WAV) $(srcdir)/simple-loop.bse $(@F).wav --seconds 5
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy  >> $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/simple-loop.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# a test for the oscillator (from toplevel test/ directory)
# FEATURE_TESTS += osc-test
EXTRA_DIST += osc-test.bse osc-test.ref
osc-test:
	$(BSE2WAV) $(srcdir)/osc-test.bse $(@F).wav --seconds 5
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/osc-test.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# a test for ArtsCompressor
FEATURE_TESTS += artscompressor-test
EXTRA_DIST += artscompressor.bse artscompressor.ref
artscompressor-test:
	$(BSE2WAV) $(srcdir)/artscompressor.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/artscompressor.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# required wave files for sample based tests *-wave-[12]-test.bse
EXTRA_DIST += pseudo-saw.bsewave pseudo-square-env.bsewave pseudo-stereo.bsewave pseudo-stereo-env.bsewave

# a test for play-type=adsr-wave-1
FEATURE_TESTS += adsr-wave-1-test
EXTRA_DIST += adsr-wave-1-test.bse adsr-wave-1-test.ref
adsr-wave-1-test:
	$(BSE2WAV) $(srcdir)/adsr-wave-1-test.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time >> $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/adsr-wave-1-test.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# a test for play-type=adsr-wave-2
FEATURE_TESTS += adsr-wave-2-test
EXTRA_DIST += adsr-wave-2-test.bse adsr-wave-2-test.ref
adsr-wave-2-test:
	$(BSE2WAV) $(srcdir)/adsr-wave-2-test.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time >> $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/adsr-wave-2-test.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# a test for play-type=plain-wave-1
FEATURE_TESTS += plain-wave-1-test
EXTRA_DIST += plain-wave-1-test.bse plain-wave-1-test.ref
plain-wave-1-test:
	$(BSE2WAV) $(srcdir)/plain-wave-1-test.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time >> $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/plain-wave-1-test.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# a test for play-type=plain-wave-2
FEATURE_TESTS += plain-wave-2-test
EXTRA_DIST += plain-wave-2-test.bse plain-wave-2-test.ref
plain-wave-2-test:
	$(BSE2WAV) $(srcdir)/plain-wave-2-test.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time >> $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/plain-wave-2-test.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav

# the BEAST demo song
FEATURE_TESTS += partymonster-test
EXTRA_DIST += partymonster.ref
partymonster-test:
	$(BSE2WAV) $(top_srcdir)/library/demo/partymonster.bse $(@F).wav
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --join-spectrum-slices=10 --avg-energy --end-time  > $(@F).tmp
	$(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --join-spectrum-slices=10 --avg-energy --end-time >> $(@F).tmp
	$(BSEFCOMPARE) $(srcdir)/partymonster.ref $(@F).tmp --threshold 99.99
	rm -f $(@F).tmp $(@F).wav 
