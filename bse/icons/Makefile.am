# BEAST & BSE
include $(top_srcdir)/Makefile.decl

# extra images to ship with tarballs
EXTRA_IMAGES = $(strip		\
	ball.xcf		\
	const.xcf		\
	disconnect.xcf		\
	instrument.xcf		\
	keyboard.xcf		\
	mic.xcf			\
	midi.xcf		\
	osc.xcf			\
	sheet.xcf		\
	snet.xcf		\
	snooper.xcf		\
	song.xcf		\
	speaker.xcf		\
	virtual.xcf		\
	waveosc.xcf		\
)

# images that need to be shipped, but no C source nees to be build from
SHIP_IMAGES = $(strip		\
)

# source images to build C source from
SRC_IMAGES = $(strip		\
	biquad.png		\
	const.png		\
	instrument.png		\
	keyboard.png		\
	mic.png			\
	midi-ctrl-input.png	\
	mono-synth.png		\
	osc.png			\
	snooper.png		\
	speaker.png		\
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
	$(AM_V_GEN)
	$(Q) $(GDK_PIXBUF_CSOURCE) --name=$(call MAKE_CNAME,$(<F))_pixstream $< >xgen-$(@F)-temp && cp xgen-$(@F)-temp $@ && rm -f xgen-$(@F)-temp
noinst_DATA = $(SRC_IMAGES:.png=.c)
CLEANFILES += $(noinst_DATA)
