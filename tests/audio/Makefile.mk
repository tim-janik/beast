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


# == check-audio ==
$(tests/audio/checks): $(tools/bsetool) FORCE		| $>/tests/audio/
check-audio: $(tests/audio/checks)
