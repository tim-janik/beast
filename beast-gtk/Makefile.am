# BEAST - Better Audio System
# Copyright (C) 1998-2002 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = gxk dialogs icons

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(BEAST_CFLAGS) -DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN
# -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_COMPAT_H

# we specify the logging domain here
DEFS += -DG_LOG_DOMAIN="\"BEAST\""

# special profiling hooks
PROFILE_SOURCE = # profile.cc
DEFS += # $(subst profile.cc, -pg -a, $(findstring profile.cc, $(<F)))
LIBS += # -lefence # -pg 

#
# setup source file variables
#
# BEAST header files that don't get installed
beast_headers = $(strip \
	bsttrackrollctrl.hh	bstxframe.hh	bstscrollgraph.hh 	\
	bstdbmeter.hh		bstbusmixer.hh	bstbuseditor.hh		bstitemseqdialog.hh  \
	bstcanvassource.hh	bstapp.hh	bstasciipixbuf.hh	bstcanvaslink.hh	    \
	bstpatterncolumns.hh	bstxkb.hh	bstpatternview.hh	bstpatternctrl.hh    \
	bstkeybindings.hh	bstprofiler.hh	bstgrowbar.hh		bstbusview.hh        \
	bstpianorollctrl.hh	bstpartview.hh	bstpianoroll.hh		bstplayback.hh	    \
	bsttrackroll.hh		bstcluehunter.hh	bstprojectctrl.hh	bstcxxutils.hh	    \
	bstauxdialogs.hh		bstsegment.hh	bsteventrollctrl.hh	bsteventroll.hh	    \
	bstsnifferscope.hh	bstwaveview.hh	bstfiledialog.hh		bstgconfig.hh	    \
	bstlogadjustment.hh	bstitemview.hh	bstservermonitor.hh	bstknob.hh	    \
	bstparamview.hh		bstmenus.hh	bstparam.hh		bstpartdialog.hh	    \
	bstprocbrowser.hh	bstqsampler.hh	bstpreferences.hh	bstprocedure.hh	    \
	bstrackeditor.hh		bstrackitem.hh	bstracktable.hh		bstsequence.hh	    \
	bstsnetrouter.hh		bstsplash.hh	bsttrackview.hh		bstsupershell.hh	    \
	bstusermessage.hh	bstdial.hh	bsttracksynthdialog.hh	bstwaveeditor.hh	    \
	bstzoomedwindow.hh	bstskinconfig.hh	bstmsgabsorb.hh		bstsampleeditor.hh   \
	bstrackview.hh		bsttreestores.hh	bstbseutils.hh		bstutils.hh	    \
	bstdefs.hh \
)
EXTRA_DIST += $(beast_headers)
# BEAST sources to build the program from
beast_sources = $(strip \
	bsttrackrollctrl.cc	bstxframe.cc	bstscrollgraph.cc	\
	bstdbmeter.cc		bstbusmixer.cc	bstbuseditor.cc		bstitemseqdialog.cc \
	bstcanvassource.cc	bstapp.cc	bstasciipixbuf.cc	bstcanvaslink.cc    \
	bstpatterncolumns.cc	bstxkb.cc	bstpatternview.cc	bstpatternctrl.cc   \
	bstkeybindings.cc	bstprofiler.cc	bstgrowbar.cc		bstbusview.cc       \
	bstpianorollctrl.cc	bstpartview.cc	bstpianoroll.cc		bstplayback.cc	    \
	bsttrackroll.cc	       bstcluehunter.cc	bstprojectctrl.cc	bstcxxutils.cc	    \
	bstauxdialogs.cc	bstsegment.cc	bsteventrollctrl.cc	bsteventroll.cc	    \
	bstsnifferscope.cc	bstwaveview.cc	bstfiledialog.cc	bstgconfig.cc	    \
	bstlogadjustment.cc	bstitemview.cc	bstservermonitor.cc	bstknob.cc	    \
	bstparamview.cc		bstmenus.cc	bstparam.cc		bstpartdialog.cc    \
	bstprocbrowser.cc	bstqsampler.cc	bstpreferences.cc	bstprocedure.cc	    \
	bstrackeditor.cc	bstrackitem.cc	bstracktable.cc		bstsequence.cc	    \
	bstsnetrouter.cc	bstsplash.cc	bsttrackview.cc		bstsupershell.cc    \
	bstusermessage.cc	bstdial.cc	bsttracksynthdialog.cc	bstwaveeditor.cc    \
	bstzoomedwindow.cc     bstskinconfig.cc	bstmsgabsorb.cc		bstsampleeditor.cc  \
	bstrackview.cc	       bsttreestores.cc	bstbseutils.cc		bstutils.cc	    \
	$(PROFILE_SOURCE) \
)
# BEAST sources that get included (don't have own .lo rules)
beast_extra_files = $(strip 						\
	bstparam-automation.cc						\
	bstparam-choice.cc		bstparam-note-sequence.cc	\
	bstparam-note-spinner.cc		bstparam-proxy.cc		\
	bstparam-color-spinner.cc	bstparam-scale.cc		\
	bstparam-searchpath.cc		bstparam-time.cc			\
	bstparam-item-seq.cc		bstrackeditor-covers.cc		\
	\
	bstmarshal.list			bstrecords.idl			\
)
EXTRA_DIST += $(beast_extra_files)

GLIB_MKENUMS = glib-mkenums
SFIDL = ../sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)

#
# rules to generate built sources
#
# bstgentypes.h
GENERATED_CLEANFILES += bstgentypes.h
CLEANFILES += stamp-bstgentypes.h
$(beast_vOBJECTS): bstgentypes.h
bstgentypes.h: stamp-bstgentypes.h ;
stamp-bstgentypes.h: $(beast_headers) $(srcdir)/bstrecords.idl $(SFIDL)
	cd .	\
	&& echo 'G_BEGIN_DECLS;'				 > xgen-$(@F) \
	&& ( cd $(srcdir) && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "#define BST_TYPE_@ENUMSHORT@\t    (bst__type_id__@EnumName@)\n" \
	  --eprod "extern GType bst__type_id__@EnumName@;" \
	    $(beast_headers) ) 					>> xgen-$(@F) \
	&& $(SFIDL) --host-c --header ${srcdir}/bstrecords.idl	>> xgen-$(@F) \
	&& echo 'G_END_DECLS;'					>> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) bstgentypes.h || cp xgen-$(@F) bstgentypes.h) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $@
# bstgentypes.cc
GENERATED_CLEANFILES += bstgentypes.cc
$(beast_vOBJECTS): bstgentypes.cc
bstgentypes.cc: bstgentypes.h $(beast_headers) ${srcdir}/bstrecords.idl $(SFIDL)
	cd . \
	&& ( cd $(srcdir) && $(GLIB_MKENUMS) \
	  --eprod "\nGType bst__type_id__@EnumName@ = 0;" \
	    $(beast_headers) ) > xgen-$(@F) \
	&& $(SFIDL) --host-c --source --init _bst_init_idl ${srcdir}/bstrecords.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bstenum_arrays.cc
GENERATED_CLEANFILES += bstenum_arrays.cc
$(beast_vOBJECTS): bstenum_arrays.cc
bstenum_arrays.cc: $(beast_headers)
	cd . \
	&& ( cd ${srcdir} && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	  --vhead "/* @EnumName@\n */\n" \
	  --vhead "static G@Type@Value @enum_name@_values[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "  { 0, NULL, NULL }\n};\n" \
	    $(beast_headers) ) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bstenum_list.cc
GENERATED_CLEANFILES += bstenum_list.cc
$(beast_vOBJECTS): bstenum_list.cc
bstenum_list.cc: $(beast_headers)
	cd . \
	&& ( cd $(srcdir) && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &bst__type_id__@EnumName@, @enum_name@_values }," \
	    $(beast_headers) ) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bstgenbseapi.h
GENERATED_CLEANFILES += bstgenbseapi.h
CLEANFILES += stamp-bstgenbseapi.h
$(beast_vOBJECTS): bstgenbseapi.h
bstgenbseapi.h: stamp-bstgenbseapi.h ;
stamp-bstgenbseapi.h: $(top_srcdir)/bse/*.idl $(top_builddir)/bse/*.idl $(SFIDL)
	cd . \
	&& $(SFIDL) $(SFIDL_INC) --client-c --header --prefix beast_ $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) bstgenbseapi.h || cp xgen-$(@F) bstgenbseapi.h) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $@
# bstgenbseapi.cc
GENERATED_CLEANFILES += bstgenbseapi.cc
$(beast_vOBJECTS): bstgenbseapi.cc
bstgenbseapi.cc: bstgenbseapi.h $(top_srcdir)/bse/*.idl $(top_builddir)/bse/*.idl $(SFIDL)
	cd .	\
	&& echo -e "/* #include \"bstgenbseapi.h\" */\n" > xgen-$(@F) \
	&& $(SFIDL) $(SFIDL_INC) --client-c --source --prefix beast_ $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) bstgenbseapi.cc \
	&& rm -f xgen-$(@F)
# bstmarshal.h
GENERATED_CLEANFILES += bstmarshal.h
$(beast_vOBJECTS): bstmarshal.h
bstmarshal.h: bstmarshal.list
	glib-genmarshal --prefix=bst_marshal $(srcdir)/bstmarshal.list --header > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bstmarshal.cc
GENERATED_CLEANFILES += bstmarshal.cc
$(srcdir)/bstutils.cc: bstmarshal.cc
bstmarshal.cc: bstmarshal.list
	glib-genmarshal --prefix=bst_marshal $(srcdir)/bstmarshal.list --body > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)


#
# convenience targets for generated source files  
#
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(GENERATED) stamp-*
generated: clean-generated $(GENERATED)


#
# build rules for programs
#
bin_PROGRAMS = beast-$(BIN_VERSION)
beast___BIN_VERSION__SOURCES = bstmain.cc $(beast_sources)
beast___BIN_VERSION__LDADD = $(progs_LDADD)
beast_vOBJECTS  = $(beast___BIN_VERSION__OBJECTS)
noinst_PROGRAMS = $(ALLTESTS)
progs_LDADD     = $(strip                                       \
	$(BEAST_LIBS) 						\
	./gxk/libgxk.a $(GXK_LIBS)                              \
	$(top_builddir)/bse/libbse.la $(BSE_LIBS)               \
)

noinst_PROGRAMS += tsmview
tsmview_SOURCES  = tsmview.cc cxxdummy.cc
tsmview_LDADD    = $(progs_LDADD)

noinst_PROGRAMS += testgui
testgui_SOURCES  = testgui.cc $(beast_sources)
testgui_LDADD    = $(progs_LDADD)
