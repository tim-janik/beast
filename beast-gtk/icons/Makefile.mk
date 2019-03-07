# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/beast-gtk/icons/*.d)
CLEANDIRS += $(wildcard $>/beast-gtk/icons/)

# == stock icons ==
beast-gtk/icons/stock.defs ::= $(strip						\
	STOCK_ARROW_DOWN		beast-gtk/icons/arrow_down.png		\
	STOCK_ARROW_LEFT		beast-gtk/icons/arrow_left.png		\
	STOCK_ARROW_RIGHT		beast-gtk/icons/arrow_right.png		\
	STOCK_ARROW_UP			beast-gtk/icons/arrow_up.png		\
	STOCK_BROWSE_IMAGE		beast-gtk/icons/browse-image.png	\
	STOCK_BUS			beast-gtk/icons/bus.png			\
	STOCK_BUS_ADD			beast-gtk/icons/bus-add.png		\
	STOCK_COLOR_SELECTOR		beast-gtk/icons/colorselector.png	\
	STOCK_DIAG			beast-gtk/icons/diag.png		\
	STOCK_EDIT			beast-gtk/icons/edit.png		\
	STOCK_EDIT_TOOL			beast-gtk/icons/editor.png		\
	STOCK_EVENT_CONTROL		beast-gtk/icons/event-control.png	\
	STOCK_FOLDER			beast-gtk/icons/folder.png		\
	STOCK_INFO			beast-gtk/icons/bulb.png		\
	STOCK_INSTRUMENT		beast-gtk/icons/aguitar.png		\
	STOCK_KNOB			beast-gtk/icons/knob.png		\
	STOCK_LADSPA			beast-gtk/icons/ladspa.png		\
	STOCK_LOAD			beast-gtk/icons/cdrom.png		\
	STOCK_LOAD_LIB			beast-gtk/icons/cdroms.png		\
	STOCK_MESH			beast-gtk/icons/mesh.png		\
	STOCK_MINI_CSYNTH		beast-gtk/icons/mini-csynth.png		\
	STOCK_MINI_MIDI_SYNTH		beast-gtk/icons/mini-midi-synth.png	\
	STOCK_MINI_SONG			beast-gtk/icons/mini-song.png		\
	STOCK_MINI_WAVE_REPO		beast-gtk/icons/mini-waverepo.png	\
	STOCK_MIXER			beast-gtk/icons/mixer.png		\
	STOCK_MOUSE_TOOL		beast-gtk/icons/mouse_tool.png		\
	STOCK_MUSIC_COPY		beast-gtk/icons/music-copy.png		\
	STOCK_MUSIC_CUT			beast-gtk/icons/music-cut.png		\
	STOCK_MUSIC_PASTE		beast-gtk/icons/music-paste.png		\
	STOCK_NOTE_1			beast-gtk/icons/note-1.png		\
	STOCK_NOTE_128			beast-gtk/icons/note-128.png		\
	STOCK_NOTE_16			beast-gtk/icons/note-16.png		\
	STOCK_NOTE_2			beast-gtk/icons/note-2.png		\
	STOCK_NOTE_32			beast-gtk/icons/note-32.png		\
	STOCK_NOTE_4			beast-gtk/icons/note-4.png		\
	STOCK_NOTE_64			beast-gtk/icons/note-64.png		\
	STOCK_NOTE_8			beast-gtk/icons/note-8.png		\
	STOCK_NOTE_ICON			beast-gtk/icons/binote.png		\
	STOCK_NO_ICON			beast-gtk/icons/noicon.png		\
	STOCK_NO_ILINK			beast-gtk/icons/no_ilink.png		\
	STOCK_NO_OLINK			beast-gtk/icons/no_olink.png		\
	STOCK_PALETTE			beast-gtk/icons/palette.png		\
	STOCK_PART			beast-gtk/icons/part.png		\
	STOCK_PART_EDITOR		beast-gtk/icons/part-editor.png		\
	STOCK_PART_LINK			beast-gtk/icons/part-link.png		\
	STOCK_PART_TEXT			beast-gtk/icons/part-text.png		\
	STOCK_PART_TOOL			beast-gtk/icons/part-tool.png		\
	STOCK_PATTERN			beast-gtk/icons/pattern.png		\
	STOCK_PATTERN_GROUP		beast-gtk/icons/pattern-group.png	\
	STOCK_PATTERN_TOOL		beast-gtk/icons/pattern-tool.png	\
	STOCK_PREVIEW_AUDIO		beast-gtk/icons/small-audio.png		\
	STOCK_PREVIEW_NO_AUDIO		beast-gtk/icons/small-noaudio.png	\
	STOCK_PROPERTIES		beast-gtk/icons/properties.png		\
	STOCK_PROPERTIES_RESET		beast-gtk/icons/properties-reset.png	\
	STOCK_QNOTE_1			beast-gtk/icons/qnote-1.png		\
	STOCK_QNOTE_128			beast-gtk/icons/qnote-128.png		\
	STOCK_QNOTE_16			beast-gtk/icons/qnote-16.png		\
	STOCK_QNOTE_2			beast-gtk/icons/qnote-2.png		\
	STOCK_QNOTE_32			beast-gtk/icons/qnote-32.png		\
	STOCK_QNOTE_4			beast-gtk/icons/qnote-4.png		\
	STOCK_QNOTE_64			beast-gtk/icons/qnote-64.png		\
	STOCK_QNOTE_8			beast-gtk/icons/qnote-8.png		\
	STOCK_QNOTE_NONE		beast-gtk/icons/qnote-none.png		\
	STOCK_QTACT			beast-gtk/icons/qtact.png		\
	STOCK_RECT_SELECT		beast-gtk/icons/rect-select.png		\
	STOCK_REPEAT			beast-gtk/icons/repeat.png		\
	STOCK_SIGNAL			beast-gtk/icons/signal.png		\
	STOCK_TARGET			beast-gtk/icons/target.png		\
	STOCK_TEXT			beast-gtk/icons/text.png		\
	STOCK_TICK_LOOP_LEFT		beast-gtk/icons/loopleft.png		\
	STOCK_TICK_LOOP_RIGHT		beast-gtk/icons/loopright.png		\
	STOCK_TICK_POINTER		beast-gtk/icons/tickpointer.png		\
	STOCK_TRACKER			beast-gtk/icons/tracker.png		\
	STOCK_TRACKS			beast-gtk/icons/tracks.png		\
	STOCK_TRACKS_ADD		beast-gtk/icons/tracks-add.png		\
	STOCK_TRASHCAN			beast-gtk/icons/trashcan.png		\
	STOCK_TRASH_SCISSORS		beast-gtk/icons/trash-scissors.png	\
	STOCK_VERT_SELECT		beast-gtk/icons/vert-select.png		\
	STOCK_WAVE			beast-gtk/icons/wave.png		\
	STOCK_WAVE_TOOL			beast-gtk/icons/wave-tool.png		\
	STOCK_ZOOM_ANY			beast-gtk/icons/zoom-any.png		\
)

beast-gtk/icons/stock.names	::= $(filter STOCK_%, $(beast-gtk/icons/stock.defs))
beast-gtk/icons/png.files	::= $(filter-out STOCK_%, $(beast-gtk/icons/stock.defs))
beast-gtk/icons/icon.deps	::= $>/beast-gtk/icons/bst-stock-gen.h
beast-gtk/icons/icon.cc.deps	::= $>/beast-gtk/icons/bst-stock-gen.cc

# == bst-stock-gen ==
$>/beast-gtk/icons/bst-stock-gen.cc: $(beast-gtk/icons/icon.deps) $(beast-gtk/icons/png.files)	| $>/beast-gtk/icons/
	$(QGEN)
	$Q $(GDK_PIXBUF_CSOURCE) --build-list $(beast-gtk/icons/stock.defs)	> $@.tmp
	$Q echo "static const GxkStockIcon stock_icons[] = {"			>>$@.tmp
	$Q for i in $(beast-gtk/icons/stock.names) ; do						\
	     echo "  { BST_$$i, $$i, },"					>>$@.tmp	\
	     || exit 1 ; done
	$Q echo "};"								>>$@.tmp
	$Q mv $@.tmp $@
$>/beast-gtk/icons/bst-stock-gen.h: $(beast-gtk/icons/png.files) beast-gtk/icons/Makefile.mk	| $>/beast-gtk/icons/
	$(QGEN)
	$Q for i in $(beast-gtk/icons/stock.names) ; do						\
	     echo "#define BST_$$i		\"BST_$$i\""			>>$@.tmp	\
	     || exit 1 ; done
	$Q mv $@.tmp $@
