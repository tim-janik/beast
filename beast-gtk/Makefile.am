# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = gxk res icons

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN="\"BEAST\""
AM_CXXFLAGS += $(BEAST_CFLAGS) $(GXK_CFLAGS) $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE \
			-DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN # -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_COMPAT_H
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
AM_LDFLAGS  += $(PYTHON_LDFLAGS)


# special profiling hooks
DEFS += @DEFINE__FILE_DIR__@ # $(subst profile.cc, -pg -a, $(findstring profile.cc, $(<F)))
LIBS += # -lefence # -pg 
AUXTYPES_PY = $(top_srcdir)/bse/AuxTypes.py

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
	bsttrackroll.hh		bstcluehunter.hh	bstprojectctrl.hh	\
	bstauxdialogs.hh		bstsegment.hh	bsteventrollctrl.hh	bsteventroll.hh	    \
	bstsnifferscope.hh	bstwaveview.hh	bstfiledialog.hh		bstgconfig.hh	    \
	bstlogadjustment.hh	bstitemview.hh	bstservermonitor.hh	bstknob.hh	    \
	bstparamview.hh		bstmenus.hh	bstparam.hh		bstpartdialog.hh	    \
	bstqsampler.hh		bstpreferences.hh	bstprocedure.hh	bstsequence.hh	    	\
	bstsnetrouter.hh		bstsplash.hh	bsttrackview.hh		bstsupershell.hh	    \
	bstusermessage.hh	bstdial.hh	bsttracksynthdialog.hh	bstwaveeditor.hh	    \
	bstzoomedwindow.hh	bstskinconfig.hh	bstmsgabsorb.hh		bstsampleeditor.hh   \
				bsttreestores.hh	bstbseutils.hh		bstutils.hh	    \
	bstdefs.hh \
	bstsoundfontview.hh	bstsoundfontpresetview.hh  \
)
EXTRA_DIST += $(beast_headers)
# BEAST sources to build the program from
bst_cc_sources = $(strip \
	bsttrackrollctrl.cc	bstxframe.cc	bstscrollgraph.cc	\
	bstdbmeter.cc		bstbusmixer.cc	bstbuseditor.cc		bstitemseqdialog.cc \
	bstcanvassource.cc	bstapp.cc	bstasciipixbuf.cc	bstcanvaslink.cc    \
	bstpatterncolumns.cc	bstxkb.cc	bstpatternview.cc	bstpatternctrl.cc   \
	bstkeybindings.cc	bstprofiler.cc	bstgrowbar.cc		bstbusview.cc       \
	bstpianorollctrl.cc	bstpartview.cc	bstpianoroll.cc		bstplayback.cc	    \
	bsttrackroll.cc	       bstcluehunter.cc	bstprojectctrl.cc	\
	bstauxdialogs.cc	bstsegment.cc	bsteventrollctrl.cc	bsteventroll.cc	    \
	bstsnifferscope.cc	bstwaveview.cc	bstfiledialog.cc	bstgconfig.cc	    \
	bstlogadjustment.cc	bstitemview.cc	bstservermonitor.cc	bstknob.cc	    \
	bstparamview.cc		bstmenus.cc	bstparam.cc		bstpartdialog.cc    \
	bstqsampler.cc	      bstpreferences.cc	bstprocedure.cc		bstsequence.cc	    \
	bstsnetrouter.cc	bstsplash.cc	bsttrackview.cc		bstsupershell.cc    \
	bstusermessage.cc	bstdial.cc	bsttracksynthdialog.cc	bstwaveeditor.cc    \
	bstzoomedwindow.cc     bstskinconfig.cc	bstmsgabsorb.cc		bstsampleeditor.cc  \
			       bsttreestores.cc	bstbseutils.cc		bstutils.cc	    \
	bstsoundfontview.cc	bstsoundfontpresetview.cc	        \
)
# BEAST sources that get included (don't have own .lo rules)
beast_extra_files = $(strip 						\
	bstparam-automation.cc						\
	bstparam-choice.cc		bstparam-note-sequence.cc	\
	bstparam-note-spinner.cc	bstparam-proxy.cc		\
	bstparam-color-spinner.cc	bstparam-scale.cc		\
	bstparam-searchpath.cc		bstparam-time.cc		\
	bstparam-item-seq.cc		bstmarshal.list			\
)
EXTRA_DIST += $(beast_extra_files)

doc_sources = $(beast_headers) $(bst_cc_sources) $(beast_extra_files)

# == doc_sources.lst ==
doc_sources.lst: Makefile
	$(AM_V_GEN)
	$(Q) echo $(patsubst %, beast-gtk/%, $(doc_sources)) > $@


SFIDL = ../sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)

#
# rules to generate built sources
#
# bstoldbseapi.h
GENERATED_CLEANFILES += bstoldbseapi.h
$(beast_build_objects): bstoldbseapi.h
bstoldbseapi.h: $(top_srcdir)/bse/*.idl $(top_builddir)/bse/*.idl # $(SFIDL)
	cd . \
	&& $(SFIDL) $(SFIDL_INC) --client-c --header --prefix beast_ $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& mv xgen-$(@F) bstoldbseapi.h
# bstoldbseapi.cc
GENERATED_CLEANFILES += bstoldbseapi.cc
$(beast_build_objects): bstoldbseapi.cc
bstoldbseapi.cc: bstoldbseapi.h $(top_srcdir)/bse/*.idl $(top_builddir)/bse/*.idl # $(SFIDL)
	cd .	\
	&& echo -e "/* #include \"bstoldbseapi.h\" */\n" > xgen-$(@F) \
	&& $(SFIDL) $(SFIDL_INC) --client-c --source --prefix beast_ $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) bstoldbseapi.cc \
	&& rm -f xgen-$(@F)
# bstmarshal.h
GENERATED_CLEANFILES += bstmarshal.h
$(beast_build_objects): bstmarshal.h
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


# == bstapi.idl ==
EXTRA_DIST += bstapi.idl
$(bst_cc_sources:%=$(srcdir)/%): bstserverapi.hh
bstserverapi.hh: $(srcdir)/bstapi.idl $(AUXTYPES_PY)
	$(AM_V_GEN)
	$(Q) ${AIDACC} -x CxxStub -G macro=BST_IDL -G serverhh -x $(AUXTYPES_PY) $< -o xgen-$(@F)
	$(Q) mv xgen-$(@F) $@
CLEANFILES += bstserverapi.hh
$(bst_cc_sources:%=$(srcdir)/%): bstserverapi.cc
bstserverapi.cc: $(srcdir)/bstapi.idl $(AUXTYPES_PY)
	$(AM_V_GEN)
	$(Q) ${AIDACC} -x CxxStub -G macro=BST_IDL -G servercc -x $(AUXTYPES_PY) $< -o xgen-$(@F)
	$(Q) mv xgen-$(@F) $@
CLEANFILES += bstserverapi.cc

#
# convenience targets for generated source files
#
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(GENERATED)
generated: clean-generated $(GENERATED)


#
# build rules for programs
#
bin_PROGRAMS = beast-@MAJOR@.@MINOR@.@MICRO@
beast_@MAJOR@_@MINOR@_@MICRO@_SOURCES = $(bst_cc_sources) bstmain.cc
nodist_beast_@MAJOR@_@MINOR@_@MICRO@_SOURCES = ../topbuildid.cc
beast_@MAJOR@_@MINOR@_@MICRO@_LDADD = $(progs_LDADD)
beast_build_objects = $(beast_@MAJOR@_@MINOR@_@MICRO@_OBJECTS) # dependency for generated sources
noinst_PROGRAMS = $(ALLTESTS)
progs_LDADD     = $(BEAST_LIBS) ./gxk/libgxk.a $(GXK_LIBS) $(top_builddir)/bse/libbse-@MAJOR@.la $(RAPICORN_LIBS) $(BSE_LIBS) $(PYTHON_LIBS) $(LIBZ_LIBS)

noinst_PROGRAMS += tsmview
tsmview_SOURCES  = tsmview.cc
tsmview_LDADD    = $(progs_LDADD)

noinst_PROGRAMS += testgui
testgui_SOURCES  = testgui.cc $(bst_cc_sources)
testgui_LDADD    = $(progs_LDADD)
