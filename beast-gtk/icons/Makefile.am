# BEAST - Bedevilled Audio System
# Copyright (C) 1998-2002 Tim Janik
#
## Makefile.am for BEAST icons

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
	text.xcf		\
	trash-scissors.xcf	\
	trashcan.xcf		\
	vert-select.xcf		\
	wave.xcf		\
	wave-tool.xcf		\
	small-audio.xcf		\
@STRIP_END@

STOCK_IMAGE_DEFS = @STRIP_BEGIN@ \
	STOCK_EDIT_TOOL			$(srcdir)/editor.png		\
	STOCK_INFO			$(srcdir)/bulb.png		\
	STOCK_INSTRUMENT		$(srcdir)/aguitar.png		\
	STOCK_KNOB			$(srcdir)/knob.png		\
	STOCK_LOAD			$(srcdir)/cdrom.png		\
	STOCK_MOUSE_TOOL		$(srcdir)/mouse_tool.png	\
	STOCK_MUSIC_CUT			$(srcdir)/music-cut.png		\
	STOCK_MUSIC_COPY		$(srcdir)/music-copy.png	\
	STOCK_MUSIC_PASTE		$(srcdir)/music-paste.png	\
	STOCK_NO_ICON			$(srcdir)/noicon.png		\
	STOCK_NO_ILINK			$(srcdir)/no_ilink.png		\
	STOCK_NO_OLINK			$(srcdir)/no_olink.png		\
	STOCK_NOTE_1			$(srcdir)/note-1.png		\
	STOCK_NOTE_2			$(srcdir)/note-2.png		\
	STOCK_NOTE_4			$(srcdir)/note-4.png		\
	STOCK_NOTE_8			$(srcdir)/note-8.png		\
	STOCK_NOTE_16			$(srcdir)/note-16.png		\
	STOCK_PALETTE			$(srcdir)/palette.png		\
	STOCK_PART			$(srcdir)/part.png		\
	STOCK_PART_EDITOR		$(srcdir)/part-editor.png	\
	STOCK_PART_TOOL			$(srcdir)/part-tool.png		\
	STOCK_PATTERN			$(srcdir)/pattern.png		\
	STOCK_PATTERN_GROUP		$(srcdir)/pattern-group.png	\
	STOCK_PATTERN_TOOL		$(srcdir)/pattern-tool.png	\
	STOCK_PREVIEW_AUDIO		$(srcdir)/small-audio.png	\
	STOCK_PREVIEW_NO_AUDIO		$(srcdir)/small-noaudio.png	\
	STOCK_QNOTE_NONE		$(srcdir)/qnote-none.png	\
	STOCK_QNOTE_1			$(srcdir)/qnote-1.png		\
	STOCK_QNOTE_2			$(srcdir)/qnote-2.png		\
	STOCK_QNOTE_4			$(srcdir)/qnote-4.png		\
	STOCK_QNOTE_8			$(srcdir)/qnote-8.png		\
	STOCK_QNOTE_16			$(srcdir)/qnote-16.png		\
	STOCK_RECT_SELECT		$(srcdir)/rect-select.png	\
	STOCK_TARGET			$(srcdir)/target.png		\
	STOCK_TEXT			$(srcdir)/text.png		\
	STOCK_TRACK			$(srcdir)/railway.png		\
	STOCK_TRASH_SCISSORS		$(srcdir)/trash-scissors.png	\
	STOCK_TRASHCAN			$(srcdir)/trashcan.png		\
	STOCK_VERT_SELECT		$(srcdir)/vert-select.png	\
	STOCK_WAVE			$(srcdir)/wave.png		\
	STOCK_WAVE_TOOL			$(srcdir)/wave-tool.png		\
	STOCK_ZOOM_ANY			$(srcdir)/zoom-any.png		\
@STRIP_END@

gen_sources = xgen-bsgc xgen-bsgh
CLEANFILES += $(gen_sources) bst-stock-gen.c bst-stock-gen.h
noinst_DATA = bst-stock-gen.c bst-stock-gen.h
EXTRA_DIST  = $(SRC_IMAGES) $(filter-out STOCK_%, $(STOCK_IMAGE_DEFS))

bst-stock-gen.c:	$(srcdir)/*.png bst-stock-gen.h
	gdk-pixbuf-csource --build-list $(STOCK_IMAGE_DEFS) > xgen-bsgc		\
	&& echo "static const GxkStockIcon stock_icons[] = {" >> xgen-bsgc	\
	&& for i in $(filter STOCK_%, $(STOCK_IMAGE_DEFS)) ; do			\
		echo "  { BST_$$i, $$i, }," >> xgen-bsgc ;			\
	done									\
	&& echo "};" >> xgen-bsgc						\
	&& cp xgen-bsgc $@							\
	&& rm -f xgen-bsgc

bst-stock-gen.h:	$(srcdir)/*.png
	for i in $(filter STOCK_%, $(STOCK_IMAGE_DEFS)) ; do			\
		echo "#define BST_$$i                \"BST_$$i\"" >> xgen-bsgh;	\
	done									\
	&& cp xgen-bsgh $@							\
	&& rm -f xgen-bsgh
