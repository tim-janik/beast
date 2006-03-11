# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998-2004 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.

# extra images to ship with tarballs
EXTRA_IMAGES = $(strip		\
	ampctrl.xcf		\
	const.xcf		\
	instrument.xcf		\
	mic.xcf			\
	osc.xcf			\
	snet.xcf		\
	song.xcf		\
	summation.xcf		\
	virtual.xcf		\
	ball.xcf		\
	disconnect.xcf		\
	keyboard.xcf		\
	midi.xcf		\
	sheet.xcf		\
	snooper.xcf		\
	speaker.xcf		\
	waveosc.xcf		\
)

# images that need to be shipped, but no C source nees to be build from
SHIP_IMAGES = $(strip		\
	ampctrl.png		\
	summation.png		\
)

# source images to build C source from
SRC_IMAGES = $(strip		\
	biquad.png		\
	const.png		\
	midi-ctrl-input.png	\
	instrument.png		\
	mono-synth.png		\
	mic.png			\
	speaker.png		\
	snooper.png		\
	osc.png			\
	keyboard.png		\
	virtual-input.png	\
	virtual-output.png	\
	virtual-synth.png	\
	waveosc.png		\
)

# function to generate C name from a filename
MAKE_CNAME = $(subst .,_,$(subst -,_,$(notdir $(basename $(1)))))

# generate .c files for all of SRC_IMAGES
GDK_PIXBUF_CSOURCE=gdk-pixbuf-csource
%.c: %.png
	$(GDK_PIXBUF_CSOURCE) --name=$(call MAKE_CNAME,$(<F))_pixstream $< >xgen-$(@F)-temp && cp xgen-$(@F)-temp $@ && rm -f xgen-$(@F)-temp
noinst_DATA = $(SRC_IMAGES:.png=.c)
CLEANFILES = $(noinst_DATA)

# ship images with tarballs
EXTRA_DIST += $(strip 	\
	$(EXTRA_IMAGES)	\
	$(SRC_IMAGES) 	\
	$(SHIP_IMAGES)	\
)
