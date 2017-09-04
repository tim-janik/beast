# BEAST & BSE
include $(top_srcdir)/Makefile.decl

# === feature test tools ===
builddirplugins = '$(top_builddir)/plugins/.libs/*.so:$(top_builddir)/plugins/freeverb/.libs/*.so'
samplepath = '$(top_srcdir)/tests/audio:$(top_srcdir)/library/samples'
BSEFEXTRACT = $(top_builddir)/tools/bsefextract
BSEFCOMPARE = $(top_builddir)/tools/bsefcompare
BSE2WAV = $(strip						\
	$(top_builddir)/bse/bsetool				\
	  $(if $(findstring 1, $(V)),, --quiet)			\
	  render2wav						\
	  --bse-mixing-freq=48000				\
	  --bse-pcm-driver null=nosleep				\
	  --bse-midi-driver null				\
	  --bse-rcfile "/dev/null"				\
	  --bse-override-plugin-globs $(builddirplugins)	\
	  --bse-override-sample-path $(samplepath)		\
	  --bse-disable-randomization				\
)

# == audio feature test rule ==
FEATURE_TESTS =
check-devel: $(FEATURE_TESTS)
# feature test temporaries
CLEANFILES += *.tmp *.wav

# === feature tests ===
FEATURE_TESTS += bseadder-test
bseadder-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/bseadder.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/bseadder.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# Test the audio-diff characteristics of BseSummation
FEATURE_TESTS += sum-diff-test
sum-diff-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/sum-diff-test.bse $(@F).wav --seconds 25
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/sum-diff-test.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += balance-test
balance-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/balance.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/balance.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += minisong-test
minisong-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/minisong.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/minisong.ref $(@F).tmp --threshold 98.00
#	$(Q) rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += syndrum-test
syndrum-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/syndrum.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/syndrum.ref $(@F).tmp --threshold 91.00
	$(Q) rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += velocity-test
velocity-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/velocity.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/velocity.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# DavOrgan module is a mono module, so we test only channel 0 (left)
FEATURE_TESTS += organsong-test
organsong-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/organsong.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/organsong.ref $(@F).tmp --threshold 98.00
	$(Q) rm -f $(@F).tmp $(@F).wav

# ADSR Test checks the mono channel envelope rendering
FEATURE_TESTS += adsrtest
adsrtest:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/adsrtest.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/adsrtest.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# checks that oscillator transposing works if the frequency is constant
FEATURE_TESTS += osctranspose1-test
osctranspose1-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/osctranspose1.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/osctranspose1.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# checks that oscillator transposing works if the frequency is a signal
FEATURE_TESTS += osctranspose2-test
osctranspose2-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/osctranspose2.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/osctranspose2.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

FEATURE_TESTS += xtalstringssong-test
xtalstringssong-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/xtalstringssong.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/xtalstringssong.ref $(@F).tmp --threshold 99.90
	$(Q) rm -f $(@F).tmp $(@F).wav

# a loop (from toplevel test/ directory)
# FEATURE_TESTS += freak-noise-test
freak-noise-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/freak-noise.bse $(@F).wav --seconds 5
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/freak-noise.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# another simple loop (from toplevel test/ directory)
# FEATURE_TESTS += simple-loop-test
simple-loop-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/simple-loop.bse $(@F).wav --seconds 5
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy  >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/simple-loop.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# a test for the oscillator (from toplevel test/ directory)
# FEATURE_TESTS += osc-test
osc-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/osc-test.bse $(@F).wav --seconds 5
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/osc-test.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# a test for ArtsCompressor
FEATURE_TESTS += artscompressor-test
artscompressor-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/artscompressor.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy  > $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/artscompressor.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# required wave files for sample based tests *-wave-[12]-test.bse
# EXTRA_DIST += pseudo-saw.bsewave pseudo-square-env.bsewave pseudo-stereo.bsewave pseudo-stereo-env.bsewave

# required soundfonts
# EXTRA_DIST += minfluid.sf2 minfluid.sf2.LICENSE

# a test for play-type=adsr-wave-1
FEATURE_TESTS += adsr-wave-1-test
adsr-wave-1-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/adsr-wave-1-test.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/adsr-wave-1-test.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# a test for play-type=adsr-wave-2
FEATURE_TESTS += adsr-wave-2-test
adsr-wave-2-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/adsr-wave-2-test.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/adsr-wave-2-test.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# a test for play-type=plain-wave-1
FEATURE_TESTS += plain-wave-1-test
plain-wave-1-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/plain-wave-1-test.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/plain-wave-1-test.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# a test for play-type=plain-wave-2
FEATURE_TESTS += plain-wave-2-test
plain-wave-2-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/plain-wave-2-test.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/plain-wave-2-test.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# a test for soundfont loading|rendering
FEATURE_TESTS += soundfont-test
soundfont-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(srcdir)/soundfont-test.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/soundfont-test.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav

# the BEAST demo song
FEATURE_TESTS += partymonster-test
partymonster-test:
	@$(AM_V_P) || echo "  WAVCHECK" $@;
	$(Q) $(BSE2WAV) $(top_srcdir)/library/demo/partymonster.bse $(@F).wav
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 0 --avg-spectrum --spectrum --join-spectrum-slices=10 --avg-energy --end-time  > $(@F).tmp
	$(Q) $(BSEFEXTRACT) $(@F).wav --cut-zeros --channel 1 --avg-spectrum --spectrum --join-spectrum-slices=10 --avg-energy --end-time >> $(@F).tmp
	$(Q) $(BSEFCOMPARE) $(srcdir)/partymonster.ref $(@F).tmp --threshold 99.99
	$(Q) rm -f $(@F).tmp $(@F).wav
