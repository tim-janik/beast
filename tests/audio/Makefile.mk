# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/tests/audio/*.d)
CLEANDIRS += $(wildcard $>/tests/audio/)
tests/audio/rpath..libbse ::= ../lib

tests/audio/checks ::=

# == render2wav ==
tests/audio/render2wav = $(strip					\
	$(tools/bsetool)						\
	  $(if $(findstring 1, $(V)),, --quiet)				\
	  render2wav							\
	  --bse-mixing-freq=48000					\
	  --bse-pcm-driver null=nosleep					\
	  --bse-midi-driver null					\
	  --bse-rcfile "/dev/null"					\
	  --bse-override-plugin-globs '$>/plugins/*.so'			\
	  --bse-override-sample-path 'tests/audio:media/Samples'	\
	  --bse-disable-randomization )

# == tests/audio/adsrtest ==
tests/audio/adsrtest: tests/audio/adsrtest.bse tests/audio/adsrtest.ref
	$(QECHO) WAVCHECK $@	# ADSR Test checks the mono channel envelope rendering
	$Q $(tests/audio/render2wav) $< $>/$@.wav
	$Q $(tools/bsetool) fextract $>/$@.wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy > $>/$@.tmp
	$Q $(tools/bsetool) fcompare $(<:%.bse=%.ref) $>/$@.tmp --threshold 99.99
	$Q rm -f $>/$@.wav $>/$@.tmp
tests/audio/checks += tests/audio/adsrtest

# == tests/audio/adsr-wave-1-test ==
tests/audio/adsr-wave-1-test: tests/audio/adsr-wave-1-test.bse tests/audio/adsr-wave-1-test.ref tests/audio/pseudo-saw.bsewave
	$(QECHO) WAVCHECK $@	# test for play-type=adsr-wave-1
	$Q $(tests/audio/render2wav) $< $>/$@.wav
	$Q $(tools/bsetool) fextract $>/$@.wav --cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time  > $>/$@.tmp
	$Q $(tools/bsetool) fextract $>/$@.wav --cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time >> $>/$@.tmp
	$Q $(tools/bsetool) fcompare $(<:%.bse=%.ref) $>/$@.tmp --threshold 99.99
	$Q rm -f $>/$@.wav $>/$@.tmp
tests/audio/checks += tests/audio/adsr-wave-1-test

# == check-audio ==
$(tests/audio/checks): $(tools/bsetool) FORCE		| $>/tests/audio/
check-audio: $(tests/audio/checks)
