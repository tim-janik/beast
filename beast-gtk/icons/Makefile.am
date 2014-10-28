# BEAST - Better Audio System
include $(top_srcdir)/Makefile.decl

SRC_IMAGES = $(strip		\
	aguitar.xcf		\
	browse-image.xcf	\
	bus-add.xcf		\
	cdrom.xcf		\
	clipboard.xcf		\
	diag.xcf		\
	editor.xcf		\
	event-control.xcf	\
	knob.xcf		\
	ladspa.xcf		\
	loopleft.xcf		\
	mesh.xcf		\
	mini-csynth.xcf		\
	mini-song.xcf		\
	mini-waverepo.xcf	\
	mixer.xcf		\
	mouse_tool.xcf		\
	no_olink.xcf		\
	noicon.xcf		\
	notes.xcf		\
	palette.xcf		\
	part.xcf		\
	part-tool.xcf		\
	pattern-group.xcf	\
	pattern-tool.xcf	\
	pattern.xcf		\
	properties.xcf		\
	railway.xcf		\
	repeat.xcf		\
	rect-select.xcf		\
	signal.xcf		\
	target.xcf		\
	text.xcf		\
	tracks-add.xcf		\
	tracks.xcf		\
	tracker.xcf		\
	trash-scissors.xcf	\
	trashcan.xcf		\
	vert-select.xcf		\
	wave.xcf		\
	wave-tool.xcf		\
	small-audio.xcf		\
)

STOCK_IMAGE_DEFS = $(strip						\
	STOCK_NOTE_ICON			$(srcdir)/binote.png		\
	STOCK_BROWSE_IMAGE		$(srcdir)/browse-image.png	\
	STOCK_BUS_ADD			$(srcdir)/bus-add.png		\
	STOCK_BUS			$(srcdir)/bus.png		\
	STOCK_COLOR_SELECTOR		$(srcdir)/colorselector.png	\
	STOCK_ARROW_UP			$(srcdir)/arrow_up.png		\
	STOCK_ARROW_LEFT		$(srcdir)/arrow_left.png	\
	STOCK_ARROW_RIGHT		$(srcdir)/arrow_right.png	\
	STOCK_ARROW_DOWN		$(srcdir)/arrow_down.png	\
	STOCK_DIAG			$(srcdir)/diag.png		\
	STOCK_EDIT			$(srcdir)/edit.png		\
	STOCK_EDIT_TOOL			$(srcdir)/editor.png		\
	STOCK_EVENT_CONTROL		$(srcdir)/event-control.png	\
	STOCK_FOLDER			$(srcdir)/folder.png		\
	STOCK_INFO			$(srcdir)/bulb.png		\
	STOCK_INSTRUMENT		$(srcdir)/aguitar.png		\
	STOCK_KNOB			$(srcdir)/knob.png		\
	STOCK_LOAD			$(srcdir)/cdrom.png		\
	STOCK_LOAD_LIB			$(srcdir)/cdroms.png		\
	STOCK_MESH			$(srcdir)/mesh.png		\
	STOCK_MINI_CSYNTH		$(srcdir)/mini-csynth.png	\
	STOCK_MINI_MIDI_SYNTH		$(srcdir)/mini-midi-synth.png	\
	STOCK_MINI_SONG			$(srcdir)/mini-song.png		\
	STOCK_MINI_WAVE_REPO		$(srcdir)/mini-waverepo.png	\
	STOCK_MIXER			$(srcdir)/mixer.png		\
	STOCK_MOUSE_TOOL		$(srcdir)/mouse_tool.png	\
	STOCK_MUSIC_CUT			$(srcdir)/music-cut.png		\
	STOCK_MUSIC_COPY		$(srcdir)/music-copy.png	\
	STOCK_MUSIC_PASTE		$(srcdir)/music-paste.png	\
	STOCK_NO_ICON			$(srcdir)/noicon.png		\
	STOCK_LADSPA			$(srcdir)/ladspa.png		\
	STOCK_NO_ILINK			$(srcdir)/no_ilink.png		\
	STOCK_NO_OLINK			$(srcdir)/no_olink.png		\
	STOCK_NOTE_1			$(srcdir)/note-1.png		\
	STOCK_NOTE_2			$(srcdir)/note-2.png		\
	STOCK_NOTE_4			$(srcdir)/note-4.png		\
	STOCK_NOTE_8			$(srcdir)/note-8.png		\
	STOCK_NOTE_16			$(srcdir)/note-16.png		\
	STOCK_NOTE_32			$(srcdir)/note-32.png		\
	STOCK_NOTE_64			$(srcdir)/note-64.png		\
	STOCK_NOTE_128			$(srcdir)/note-128.png		\
	STOCK_PALETTE			$(srcdir)/palette.png		\
	STOCK_PART			$(srcdir)/part.png		\
	STOCK_PART_TEXT			$(srcdir)/part-text.png		\
	STOCK_PART_LINK			$(srcdir)/part-link.png		\
	STOCK_PART_EDITOR		$(srcdir)/part-editor.png	\
	STOCK_PART_TOOL			$(srcdir)/part-tool.png		\
	STOCK_PATTERN			$(srcdir)/pattern.png		\
	STOCK_PATTERN_GROUP		$(srcdir)/pattern-group.png	\
	STOCK_PATTERN_TOOL		$(srcdir)/pattern-tool.png	\
	STOCK_PREVIEW_AUDIO		$(srcdir)/small-audio.png	\
	STOCK_PREVIEW_NO_AUDIO		$(srcdir)/small-noaudio.png	\
	STOCK_PROPERTIES		$(srcdir)/properties.png	\
	STOCK_PROPERTIES_RESET		$(srcdir)/properties-reset.png	\
	STOCK_QTACT			$(srcdir)/qtact.png		\
	STOCK_QNOTE_NONE		$(srcdir)/qnote-none.png	\
	STOCK_QNOTE_1			$(srcdir)/qnote-1.png		\
	STOCK_QNOTE_2			$(srcdir)/qnote-2.png		\
	STOCK_QNOTE_4			$(srcdir)/qnote-4.png		\
	STOCK_QNOTE_8			$(srcdir)/qnote-8.png		\
	STOCK_QNOTE_16			$(srcdir)/qnote-16.png		\
	STOCK_QNOTE_32			$(srcdir)/qnote-32.png		\
	STOCK_QNOTE_64			$(srcdir)/qnote-64.png		\
	STOCK_QNOTE_128			$(srcdir)/qnote-128.png		\
	STOCK_RECT_SELECT		$(srcdir)/rect-select.png	\
	STOCK_REPEAT			$(srcdir)/repeat.png		\
	STOCK_SIGNAL			$(srcdir)/signal.png		\
	STOCK_TARGET			$(srcdir)/target.png		\
	STOCK_TEXT			$(srcdir)/text.png		\
	STOCK_TICK_LOOP_LEFT		$(srcdir)/loopleft.png		\
	STOCK_TICK_LOOP_RIGHT		$(srcdir)/loopright.png		\
	STOCK_TICK_POINTER		$(srcdir)/tickpointer.png	\
	STOCK_TRACKS			$(srcdir)/tracks.png		\
	STOCK_TRACKS_ADD		$(srcdir)/tracks-add.png	\
	STOCK_TRACKER			$(srcdir)/tracker.png		\
	STOCK_TRASH_SCISSORS		$(srcdir)/trash-scissors.png	\
	STOCK_TRASHCAN			$(srcdir)/trashcan.png		\
	STOCK_VERT_SELECT		$(srcdir)/vert-select.png	\
	STOCK_WAVE			$(srcdir)/wave.png		\
	STOCK_WAVE_TOOL			$(srcdir)/wave-tool.png		\
	STOCK_ZOOM_ANY			$(srcdir)/zoom-any.png		\
)

gen_sources = xgen-bsgc xgen-bsgh
CLEANFILES += $(gen_sources) bst-stock-gen.cc bst-stock-gen.h
noinst_DATA = bst-stock-gen.cc bst-stock-gen.h
EXTRA_DIST += $(SRC_IMAGES) $(filter-out STOCK_%, $(STOCK_IMAGE_DEFS))

bst-stock-gen.cc: $(srcdir)/*.png bst-stock-gen.h
	gdk-pixbuf-csource --build-list $(STOCK_IMAGE_DEFS) > xgen-bsgc		\
	&& echo "static const GxkStockIcon stock_icons[] = {" >> xgen-bsgc	\
	&& for i in $(filter STOCK_%, $(STOCK_IMAGE_DEFS)) ; do			\
		echo "  { BST_$$i, $$i, }," >> xgen-bsgc ;			\
	done									\
	&& echo "};" >> xgen-bsgc						\
	&& cp xgen-bsgc $@							\
	&& rm -f xgen-bsgc

bst-stock-gen.h: $(srcdir)/*.png
	for i in $(filter STOCK_%, $(STOCK_IMAGE_DEFS)) ; do			\
		echo "#define BST_$$i                \"BST_$$i\"" >> xgen-bsgh;	\
	done									\
	&& cp xgen-bsgh $@							\
	&& rm -f xgen-bsgh
