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
	  --bse-pcm-driver null						\
	  --bse-midi-driver null					\
	  --bse-override-plugin-globs '$>/plugins/*.so'			\
	  --bse-override-sample-path 'tests/audio:media/Samples'	\
	  --bse-disable-randomization					\
	  --bse-rcfile /dev/null )
tests/audio/plugin.deps = $(plugins/cxxplugins.so) $(plugins/bseplugins.so) $(plugins/freeverb.so)

# == tests/audio/template ==
# $(call tests/audio/template, 1TESTNAME, 2BSE+REF+DEPS, 3RENDERARGS, 4FEATURES-A, 5FEATURES-B, 6THRESHOLD)
define tests/audio/template.impl
$1: $2 FORCE
	$$(QECHO) WAVCHECK $$@
	$$Q $$(tests/audio/render2wav) $$< $3 $$>/$$@.wav
	$$Q $$(if $4, $$(tools/bsetool) fextract $$>/$$@.wav $4) > $$>/$$@.tmp
	$$Q $$(if $5, $$(tools/bsetool) fextract $$>/$$@.wav $5  >>$$>/$$@.tmp )
	$$Q $$(tools/bsetool) fcompare $(if $(findstring 1, $(V)),--verbose) $$(word 2,$$^) $$>/$$@.tmp --threshold $6
	$$Q rm -f $$>/$$@.wav $$>/$$@.tmp
tests/audio/checks += $1
endef
tests/audio/template = $(eval $(call tests/audio/template.impl,$1,$2,$3,$4,$5,$6))

# == tests/audio/adsrtest ==
$(call tests/audio/template,							\
	tests/audio/adsrtest,							\
	tests/audio/adsrtest.bse tests/audio/adsrtest.ref, ,			\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy, ,	\
	99.99)	# ADSR Test checks the mono channel envelope rendering

# == tests/audio/adsr-wave-1-test ==
$(call tests/audio/template,							\
	tests/audio/adsr-wave-1-test,						\
	tests/audio/adsr-wave-1-test.bse tests/audio/adsr-wave-1-test.ref tests/audio/pseudo-saw.bsewave, , \
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time, \
	--cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time, \
	99.99)	# test for play-type=adsr-wave-1

# == tests/audio/adsr-wave-2-test ==
$(call tests/audio/template,							\
	tests/audio/adsr-wave-2-test,						\
	tests/audio/adsr-wave-2-test.bse tests/audio/adsr-wave-2-test.ref tests/audio/pseudo-stereo.bsewave, , \
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time, \
	--cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time, \
	99.99)	# test for play-type=adsr-wave-2

# == tests/audio/artscompressor ==
$(call tests/audio/template,							\
	tests/audio/artscompressor,						\
	tests/audio/artscompressor.bse tests/audio/artscompressor.ref, ,	\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy, ,	\
	99.99)

# == tests/audio/bseadder ==
$(call tests/audio/template,							\
	tests/audio/bseadder,							\
	tests/audio/bseadder.bse tests/audio/bseadder.ref, ,			\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy, ,	\
	99.99)

# == tests/audio/balance ==
$(call tests/audio/template,							\
	tests/audio/balance,							\
	tests/audio/balance.bse tests/audio/balance.ref, ,			\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy,		\
	--cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy,		\
	99.99)

# == tests/audio/freak-noise ==
$(call tests/audio/template,							\
	tests/audio/freak-noise,						\
	tests/audio/freak-noise.bse tests/audio/freak-noise.ref,		\
	--seconds 5,								\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy, ,	\
	99.70)	# simple loop

# == tests/audio/minisong ==
$(call tests/audio/template,							\
	tests/audio/minisong,							\
	tests/audio/minisong.bse tests/audio/minisong.ref, ,			\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy,		\
	--cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy,		\
	98.00)

# == tests/audio/organsong ==
$(call tests/audio/template,							\
	tests/audio/organsong,							\
	tests/audio/organsong.bse tests/audio/organsong.ref, ,			\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy, ,	\
	98.00)	# test DavOrgan mono voice module

# == tests/audio/osc-test ==
$(call tests/audio/template,							\
	tests/audio/osc-test,							\
	tests/audio/osc-test.bse tests/audio/osc-test.ref,			\
	--seconds 5,								\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy, ,	\
	99.90)	# a test for the oscillator

# == tests/audio/osctranspose1 ==
$(call tests/audio/template,							\
	tests/audio/osctranspose1,						\
	tests/audio/osctranspose1.bse tests/audio/osctranspose1.ref, ,		\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy, ,	\
	99.99)	# checks that oscillator transposing works if the frequency is constant

# == tests/audio/osctranspose2 ==
$(call tests/audio/template,							\
	tests/audio/osctranspose2,						\
	tests/audio/osctranspose2.bse tests/audio/osctranspose2.ref, ,		\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy, ,	\
	99.99)	# checks that oscillator transposing works if the frequency is a signal

# == tests/audio/partymonster ==
$(call tests/audio/template,							\
	tests/audio/partymonster,						\
	media/Demos/partymonster.bse tests/audio/partymonster.ref, ,		\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --join-spectrum-slices=10 --avg-energy --end-time, \
	--cut-zeros --channel 1 --avg-spectrum --spectrum --join-spectrum-slices=10 --avg-energy --end-time, \
	99.99)	# Beast demo song

# == tests/audio/plain-wave-1-test ==
$(call tests/audio/template,							\
	tests/audio/plain-wave-1-test,						\
	tests/audio/plain-wave-1-test.bse tests/audio/plain-wave-1-test.ref	\
		tests/audio/pseudo-square-env.bsewave, , \
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time, \
	--cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time, \
	99.99)	# test for play-type=plain-wave-1
# FIXME: sometimes warns about missing 'note-off'

# == tests/audio/plain-wave-2-test ==
$(call tests/audio/template,							\
	tests/audio/plain-wave-2-test,						\
	tests/audio/plain-wave-2-test.bse tests/audio/plain-wave-2-test.ref	\
		tests/audio/pseudo-stereo-env.bsewave, , \
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time, \
	--cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time, \
	99.99)	# test for play-type=plain-wave-2

# == tests/audio/simple-loop ==
$(call tests/audio/template,							\
	tests/audio/simple-loop,						\
	tests/audio/simple-loop.bse tests/audio/simple-loop.ref,		\
	--seconds 5,								\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy,		\
	--cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy,		\
	97.99)	# another simple loop

# == tests/audio/soundfont-test ==
$(call tests/audio/template,							\
	tests/audio/soundfont-test,						\
	tests/audio/soundfont-test.bse tests/audio/soundfont-test.ref		\
		tests/audio/minfluid.sf2 tests/audio/minfluid.sf2.LICENSE, ,	\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy --end-time, \
	--cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy --end-time, \
	99.99)	# test for soundfont loading & rendering
# NOTE: needs fluidsynth >= 1.1.7 which contains a modulator bugfix, FluidSynth/fluidsynth#194

# == tests/audio/sum-diff-test ==
$(call tests/audio/template,							\
	tests/audio/sum-diff-test,						\
	tests/audio/sum-diff-test.bse tests/audio/sum-diff-test.ref,		\
	--seconds 25,								\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy, ,	\
	99.80)

# == tests/audio/syndrum ==
$(call tests/audio/template,							\
	tests/audio/syndrum,							\
	tests/audio/syndrum.bse tests/audio/syndrum.ref, ,			\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy,		\
	--cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy,		\
	91.00)

# == tests/audio/velocity ==
$(call tests/audio/template,							\
	tests/audio/velocity,							\
	tests/audio/velocity.bse tests/audio/velocity.ref, ,			\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy,		\
	--cut-zeros --channel 1 --avg-spectrum --spectrum --avg-energy,		\
	99.99)

# == tests/audio/xtalstringssong ==
$(call tests/audio/template,							\
	tests/audio/xtalstringssong,						\
	tests/audio/xtalstringssong.bse tests/audio/xtalstringssong.ref, ,	\
	--cut-zeros --channel 0 --avg-spectrum --spectrum --avg-energy, ,	\
	99.90)

# == check-audio ==
$(tests/audio/checks): $(tools/bsetool) $(tests/audio/plugin.deps) FORCE		| $>/tests/audio/
check-audio: $(tests/audio/checks)
