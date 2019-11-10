# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/bse/icons/*.d)
CLEANDIRS += $(wildcard $>/bse/icons/)

# == icons/ files ==
bse/icons/png.files ::= $(strip		\
	bse/icons/biquad.png		\
	bse/icons/const.png		\
	bse/icons/instrument.png	\
	bse/icons/keyboard.png		\
	bse/icons/mic.png		\
	bse/icons/midi-ctrl-input.png	\
	bse/icons/mono-synth.png	\
	bse/icons/osc.png		\
	bse/icons/speaker.png		\
	bse/icons/virtual-input.png	\
	bse/icons/virtual-output.png	\
	bse/icons/virtual-synth.png	\
	bse/icons/waveosc.png		\
)
bse/icons/c.csources ::= $(sort $(bse/icons/png.files:%.png=$>/%.c))

# == mkident for icons ==
# function to generate C name from a filename
bse/icons/icons.mkident = $(subst .,_,$(subst -,_,$(notdir $(basename $(1)))))

# == icons/*.c rules ==
$>/bse/icons/%.c: bse/icons/%.png		| $>/bse/icons/
	$(QGEN)
	$Q $(GDK_PIXBUF_CSOURCE) --name=$(call bse/icons/icons.mkident,$(<F))_pixstream $<	> $@.tmp
	$Q mv $@.tmp $@
