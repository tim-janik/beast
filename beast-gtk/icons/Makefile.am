# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BEAST icons

EXTRA_DIST += @STRIP_BEGIN@ \
	$(icon_list) \
@STRIP_END@

SRC_IMAGES = @STRIP_BEGIN@	\
	aguitar.xcf		\
	cdrom.xcf		\
	editor.xcf		\
	knob.xcf		\
	mouse_tool.xcf		\
	no_olink.xcf		\
	noicon.xcf		\
	palette.xcf		\
	part-tool.xcf		\
	pattern-group.xcf	\
	pattern-tool.xcf	\
	pattern.xcf		\
	rect-select.xcf		\
	target.xcf		\
	trashcan.xcf		\
	vert-select.xcf		\
	wave.xcf		\
	wave-tool.xcf		\
	small-audio.xcf		\
@STRIP_END@

STOCK_IMAGE_PAIRS = @STRIP_BEGIN@ \
	stock_info			$(srcdir)/bulb.png		\
	stock_cdrom			$(srcdir)/cdrom.png		\
	stock_editor			$(srcdir)/editor.png		\
	stock_instrument		$(srcdir)/aguitar.png		\
	stock_knob			$(srcdir)/knob.png		\
	stock_mouse_tool		$(srcdir)/mouse_tool.png	\
	stock_no_ilink			$(srcdir)/no_ilink.png		\
	stock_no_olink			$(srcdir)/no_olink.png		\
	stock_no_icon			$(srcdir)/noicon.png		\
	stock_palette			$(srcdir)/palette.png		\
	stock_part			$(srcdir)/part.png		\
	stock_part_editor		$(srcdir)/part-editor.png	\
	stock_part_tool			$(srcdir)/part-tool.png		\
	stock_pattern_group		$(srcdir)/pattern-group.png	\
	stock_pattern_tool		$(srcdir)/pattern-tool.png	\
	stock_pattern			$(srcdir)/pattern.png		\
	stock_rect_select		$(srcdir)/rect-select.png	\
	stock_small_audio		$(srcdir)/small-audio.png	\
	stock_small_noaudio		$(srcdir)/small-noaudio.png	\
	stock_target			$(srcdir)/target.png		\
	stock_trashcan			$(srcdir)/trashcan.png		\
	stock_vert_select		$(srcdir)/vert-select.png	\
	stock_wave			$(srcdir)/wave.png		\
	stock_wave_tool			$(srcdir)/wave-tool.png		\
	stock_zoom_any			$(srcdir)/zoom-any.png		\
@STRIP_END@

gen_sources           = xgen-bstg
CLEANFILES           += $(gen_sources)
MAINTAINERCLEANFILES += bst-stock-gen.c
noinst_DATA           = bst-stock-gen.c

bst-stock-gen.c:	*.png
	gdk-pixbuf-csource --build-list $(STOCK_IMAGE_PAIRS) >xgen-bstg && \
	mv xgen-bstg $@

EXTRA_DIST = bst-stock-gen.c
