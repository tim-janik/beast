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
	clipboard.xcf		\
	editor.xcf		\
	knob.xcf		\
	mouse_tool.xcf		\
	no_olink.xcf		\
	noicon.xcf		\
	notes.xcf		\
	palette.xcf		\
	part-tool.xcf		\
	pattern-group.xcf	\
	pattern-tool.xcf	\
	pattern.xcf		\
	railway.xcf		\
	rect-select.xcf		\
	target.xcf		\
	trash-scissors.xcf	\
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
	stock_music_cut			$(srcdir)/music-cut.png		\
	stock_music_copy		$(srcdir)/music-copy.png	\
	stock_music_paste		$(srcdir)/music-paste.png	\
	stock_no_ilink			$(srcdir)/no_ilink.png		\
	stock_no_olink			$(srcdir)/no_olink.png		\
	stock_no_icon			$(srcdir)/noicon.png		\
	stock_note_1			$(srcdir)/note-1.png		\
	stock_note_2			$(srcdir)/note-2.png		\
	stock_note_4			$(srcdir)/note-4.png		\
	stock_note_8			$(srcdir)/note-8.png		\
	stock_note_16			$(srcdir)/note-16.png		\
	stock_palette			$(srcdir)/palette.png		\
	stock_part			$(srcdir)/part.png		\
	stock_part_editor		$(srcdir)/part-editor.png	\
	stock_part_tool			$(srcdir)/part-tool.png		\
	stock_pattern_group		$(srcdir)/pattern-group.png	\
	stock_pattern_tool		$(srcdir)/pattern-tool.png	\
	stock_pattern			$(srcdir)/pattern.png		\
	stock_qnote_none		$(srcdir)/qnote-none.png	\
	stock_qnote_1			$(srcdir)/qnote-1.png		\
	stock_qnote_2			$(srcdir)/qnote-2.png		\
	stock_qnote_4			$(srcdir)/qnote-4.png		\
	stock_qnote_8			$(srcdir)/qnote-8.png		\
	stock_qnote_16			$(srcdir)/qnote-16.png		\
	stock_railway			$(srcdir)/railway.png		\
	stock_rect_select		$(srcdir)/rect-select.png	\
	stock_small_audio		$(srcdir)/small-audio.png	\
	stock_small_noaudio		$(srcdir)/small-noaudio.png	\
	stock_target			$(srcdir)/target.png		\
	stock_trashcan			$(srcdir)/trashcan.png		\
	stock_trash_scissors		$(srcdir)/trash-scissors.png	\
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
