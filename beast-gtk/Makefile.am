# BEAST - Bedevilled Audio System
# Copyright (C) 1998-2002 Tim Janik
#
## Makefile.am for BEAST
 
SUBDIRS = gxk dialogs icons

INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(BEAST_CFLAGS) -DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN
# -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_COMPAT_H

# we specify the logging domain here
DEFS += $(strip 			\
	-DG_LOG_DOMAIN="\"BST\"" 	\
)

# special profiling hooks
PROFILE_SOURCE = # profile.c
CFLAGS += # $(subst profile.c, -pg -a, $(findstring profile.c, $(<F)))
LIBS += # -lefence # -pg 

# link programs against BSE and other neccessary libraries
progs_LDADD = $(strip 			\
	./gxk/libgxk.a 			\
	$(top_builddir)/bse/libbse.la 	\
	$(top_builddir)/sfi/libsfi.la 	\
	$(LDFLAGS) $(BEAST_LIBS) 	\
)

#
# setup source file variables
#
# BEAST header files that don't get installed
beast_headers = $(strip \
	bsttrackrollctrl.h	bstxframe.h	bstscrollgraph.h 	\
	bstdbmeter.h		bstbusmixer.h	bstbuseditor.h		bstitemseqdialog.h  \
	bstcanvassource.h	bstapp.h	bstasciipixbuf.h	bstcanvaslink.h	    \
	bstpatterncolumns.h	bstxkb.h	bstpatternview.h	bstpatternctrl.h    \
	bstkeybindings.h	bstprofiler.h	bstgrowbar.h		bstbusview.h        \
	bstpianorollctrl.h	bstpartview.h	bstpianoroll.h		bstplayback.h	    \
	bsttrackroll.h		bstcluehunter.h	bstprojectctrl.h	bstxxutils.hh	    \
	bstauxdialogs.h		bstsegment.h	bsteventrollctrl.h	bsteventroll.h	    \
	bstsnifferscope.h	bstwaveview.h	bstfiledialog.h		bstgconfig.h	    \
	bstlogadjustment.h	bstitemview.h	bstservermonitor.h	bstknob.h	    \
	bstparamview.h		bstmenus.h	bstparam.h		bstpartdialog.h	    \
	bstprocbrowser.h	bstqsampler.h	bstpreferences.h	bstprocedure.h	    \
	bstrackeditor.h		bstrackitem.h	bstracktable.h		bstsequence.h	    \
	bstsnetrouter.h		bstsplash.h	bsttrackview.h		bstsupershell.h	    \
	bstusermessage.h	bstdial.h	bsttracksynthdialog.h	bstwaveeditor.h	    \
	bstzoomedwindow.h	bstskinconfig.h	bstmsgabsorb.h		bstsampleeditor.h   \
	bstrackview.h		bsttreestores.h	bstbseutils.h		bstutils.h	    \
	bstmarshal.h bstdefs.h \
)
# BEAST c sources to build the program from
beast_sources = $(strip \
	bsttrackrollctrl.c	bstxframe.c	bstscrollgraph.c	\
	bstdbmeter.c		bstbusmixer.c	bstbuseditor.c		bstitemseqdialog.c  \
	bstcanvassource.c	bstapp.c	bstasciipixbuf.c	bstcanvaslink.c	    \
	bstpatterncolumns.c	bstxkb.c	bstpatternview.c	bstpatternctrl.c    \
	bstkeybindings.c	bstprofiler.c	bstgrowbar.c		bstbusview.c        \
	bstpianorollctrl.c	bstpartview.c	bstpianoroll.c		bstplayback.c	    \
	bsttrackroll.c		bstcluehunter.c	bstprojectctrl.c	bstxxutils.cc	    \
	bstauxdialogs.c		bstsegment.c	bsteventrollctrl.c	bsteventroll.c	    \
	bstsnifferscope.c	bstwaveview.c	bstfiledialog.c		bstgconfig.c	    \
	bstlogadjustment.c	bstitemview.c	bstservermonitor.c	bstknob.c	    \
	bstparamview.c		bstmenus.c	bstparam.c		bstpartdialog.c	    \
	bstprocbrowser.c	bstqsampler.c	bstpreferences.c	bstprocedure.c	    \
	bstrackeditor.c		bstrackitem.c	bstracktable.c		bstsequence.c	    \
	bstsnetrouter.c		bstsplash.c	bsttrackview.c		bstsupershell.c	    \
	bstusermessage.c	bstdial.c	bsttracksynthdialog.c	bstwaveeditor.c	    \
	bstzoomedwindow.c	bstskinconfig.c	bstmsgabsorb.c		bstsampleeditor.c   \
	bstrackview.c		bsttreestores.c	bstbseutils.c		bstutils.c	    \
	$(PROFILE_SOURCE) \
)
# BEAST sources that get included (don't have own .lo rules)
beast_extra_sources = $(strip 						\
	bstparam-automation.c						\
	bstparam-choice.c		bstparam-note-sequence.c	\
	bstparam-note-spinner.c		bstparam-proxy.c		\
	bstparam-color-spinner.c	bstparam-scale.c		\
	bstparam-searchpath.c		bstparam-time.c			\
	bstparam-item-seq.c		bstrackeditor-covers.c		\
)
beast_extra_files = $(strip \
	bstmarshal.list					\
	bstdebugkeys.defs	bstrecords.idl		\
)
# BEAST built sources needed by all objects
beast_built_depend_sources = $(strip \
	bstmarshal.h	\
	bstgentypes.h	\
	bstgenbseapi.h	\
)
# BEAST built sources
beast_built_sources = $(strip 		\
	$(beast_built_depend_sources)	\
	bstmarshal.c			\
	bstgentypes.c 			\
	bstgenbseapi.c 			\
	bstenum_arrays.c		\
	bstenum_list.c 			\
)
# BEAST stamp files for built-sources generation
beast_built_stamp = $(strip		\
	stamp-bstenum_arrays.c		\
	stamp-bstgenbseapi.h		\
)
# built source have to be cleaned with stamps to force proper regeneration
CLEANFILES += $(beast_built_sources) $(beast_built_stamp)
# rules to build built-sources
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(beast_built_sources) $(beast_built_stamp)
generated: clean-generated $(beast_built_sources)

GLIB_MKENUMS = glib-mkenums

SFIDL = ../sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)

# gen-sources
CLEANFILES += xgen-beac xgen-beac xgen-bgth xgen-bgtc xgen-belc xgen-bmh xgen-bmc xgen-bdkd
bstenum_arrays.c: $(beast_headers)
	cd . \
	&& ( cd ${srcdir} && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	  --vhead "/* @EnumName@\n */\n" \
	  --vhead "static G@Type@Value @enum_name@_values[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "  { 0, NULL, NULL }\n};\n" \
	    $(beast_headers) ) > xgen-beac \
	&& (cmp -s xgen-beac $@ || touch stamp-bstenum_arrays.c) \
	&& cp xgen-beac $@ \
	&& rm -f xgen-beac
stamp-bstenum_arrays.c: bstenum_arrays.c
	@ [ -f $@ ] || touch $@
bstgentypes.h: stamp-bstenum_arrays.c $(srcdir)/bstrecords.idl $(SFIDL)
	cd .	\
	&& ( cd $(srcdir) && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "#define BST_TYPE_@ENUMSHORT@\t    (bst__type_id__@EnumName@)\n" \
	  --eprod "extern GType bst__type_id__@EnumName@;" \
	    $(beast_headers) ) > xgen-bgth \
	&& $(SFIDL) --host-c --header ${srcdir}/bstrecords.idl >> xgen-bgth \
	&& cp xgen-bgth $@ \
	&& rm -f xgen-bgth
$(srcdir)/bstutils.c:	bstgentypes.c
bstgentypes.c: stamp-bstenum_arrays.c ${srcdir}/bstrecords.idl $(SFIDL)
	cd . \
	&& ( cd $(srcdir) && $(GLIB_MKENUMS) \
	  --eprod "\nGType bst__type_id__@EnumName@ = 0;" \
	    $(beast_headers) ) > xgen-bgtc \
	&& $(SFIDL) --host-c --source --init _bst_init_idl ${srcdir}/bstrecords.idl >> xgen-bgtc \
        && cp xgen-bgtc $@ \
	&& rm -f xgen-bgtc
$(srcdir)/bstutils.c:	bstenum_list.c
bstenum_list.c: stamp-bstenum_arrays.c
	cd . \
	&& ( cd $(srcdir) && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &bst__type_id__@EnumName@, @enum_name@_values }," \
	    $(beast_headers) ) > xgen-belc \
	&& cp xgen-belc $@ \
	&& rm -f xgen-belc
bstgenbseapi.h: stamp-bstgenbseapi.h ;
stamp-bstgenbseapi.h: $(top_srcdir)/bse/*.idl $(top_builddir)/bse/*.idl $(SFIDL)
	cd .	\
	&& echo -e "#include <sfi/sfi.h>\n" > xgen-bgbah \
	&& $(SFIDL) $(SFIDL_INC) --client-c --header --prefix beast_ $(top_srcdir)/bse/bse.idl >> xgen-bgbah \
	&& (cmp -s xgen-bgbah bstgenbseapi.h || cp xgen-bgbah bstgenbseapi.h) \
	&& rm -f xgen-bgbah \
	&& echo timestamp > $@
$(srcdir)/bstbseutils.c: bstgenbseapi.c
bstgenbseapi.c: bstgenbseapi.h $(top_srcdir)/bse/*.idl $(top_builddir)/bse/*.idl $(SFIDL)
	cd .	\
	&& echo -e "/* #include \"bstgenbseapi.h\" */\n" > xgen-bgbac \
	&& $(SFIDL) $(SFIDL_INC) --client-c --source --prefix beast_ $(top_srcdir)/bse/bse.idl >> xgen-bgbac \
	&& cp xgen-bgbac bstgenbseapi.c \
	&& rm -f xgen-bgbac
bstmarshal.h: bstmarshal.list
	glib-genmarshal --prefix=bst_marshal $(srcdir)/bstmarshal.list --header > xgen-bmh \
	&& cp xgen-bmh $@ \
	&& rm -f xgen-bmh
$(srcdir)/bstutils.c:	bstmarshal.c
bstmarshal.c: bstmarshal.list
	glib-genmarshal --prefix=bst_marshal $(srcdir)/bstmarshal.list --body > xgen-bmc \
	&& cp xgen-bmc $@ \
	&& rm -f xgen-bmc

SCANKEYS=$(TEXITHEQUE) scankeys --

debug_key_globs = $(top_srcdir)/beast-gtk/*.[hc] $(top_srcdir)/bse/*.[hc] $(top_srcdir)/bse/*.cc $(top_srcdir)/sfi/*.[hc] $(top_srcdir)/sfi/*.cc
bstdebugkeys.defs: @DVLBUILD@ $(debug_key_globs)
	cd $(srcdir) \
	&& echo > xgen-bdkd \
	&& grep -E 'sfi_debug(|_check)\b' $(debug_key_globs) \
	| $(SCANKEYS) --key sfi_debug --key sfi_log_check \
	     --tmplkey   '  %s,\n' \
	     --needkeys - \
	| sort >> xgen-bdkd \
	&& cp xgen-bdkd $@ \
	&& rm -f xgen-bdkd
$(srcdir)/bstutils.c: bstdebugkeys.defs

#
# setup BEAST sources and their dependancies
#
bin_PROGRAMS = @BEAST_BINARY@			# this expands to beast-$BIN_VERSION
EXTRA_PROGRAMS = beast-\$(BIN_VERSION)		# make automake-1.4 eat beast____BIN_VERSION_ variables
beast____BIN_VERSION__SOURCES = bstmain.c $(beast_sources)
beast____BIN_VERSION__LDADD = $(progs_LDADD)
beast_objects = $(beast____BIN_VERSION__OBJECTS)
noinst_PROGRAMS = tsmview testgui # bststest
tsmview_SOURCES = tsmview.c cxxdummy.cc
tsmview_LDADD = $(progs_LDADD)
testgui_SOURCES = testgui.c $(beast_sources)
testgui_LDADD = $(progs_LDADD)
# bststest_SOURCES = bststest.c bstutils.c bstqsampler.c cxxdummy.cc
# bststest_LDADD = $(progs_LDADD)


#
# rules to generate built sources
#
#$(beast_objects) $(bststest_OBJECTS) $(tsmview_OBJECTS): $(beast_built_depend_sources)
$(OBJECTS): $(beast_built_depend_sources)

# broken: $(beast_SOURCES) $(bststest_SOURCES) $(tsmview_SOURCES): bstgentypes.h
# broken: EXTRA_DIST += $(beast_built_sources)
# using _SOURCES here, cause make to attempt to remake .c files, confusing
# it about vpaths:
#-      No commands for `bstmain.c' and no prerequisites actually changed.
#-      No need to remake target `bstmain.c'; using VPATH name `../../beast-gtk/bstmain.c'.
#-      Pruning file `../../beast-gtk/bstmain.c'.
#+      Must remake target `bstmain.c'.
#+      Ignoring VPATH name `../../beast-gtk/bstmain.c'.
#+      Successfully remade target file `bstmain.c'.
#+      Pruning file `bstmain.c'.


EXTRA_DIST += $(beast_headers) $(beast_extra_sources) $(beast_extra_files)

#
# private (birnet) dependancy to make TAGS file after each build
#
all: # TAGS

