# BEAST & BSE

include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)

SUBDIRS = gxk icons

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += -DG_LOG_DOMAIN="\"BEAST\""
AM_CXXFLAGS += $(BEAST_CFLAGS) $(GXK_CFLAGS) $(BSE_CFLAGS) $(RAPICORN_CFLAGS) \
			-DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN # -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_COMPAT_H
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
AM_LDFLAGS  += $(PYTHON_LDFLAGS)
AIDACC       = $(topdir)/aidacc/aidacc


# special profiling hooks
DEFS += # $(subst profile.cc, -pg -a, $(findstring profile.cc, $(<F)))
LIBS += # -lefence # -pg 

#
# setup source file variables
#
# BEAST header files that don't get installed
beast_headers = $(strip \
	bstapp.hh			\
	bstasciipixbuf.hh		\
	bstauxdialogs.hh		\
	bstbseutils.hh			\
	bstbuseditor.hh			\
	bstbusmixer.hh			\
	bstbusview.hh			\
	bstcanvaslink.hh		\
	bstcanvassource.hh		\
	bstcluehunter.hh		\
	bstdbmeter.hh			\
	bstdefs.hh			\
	bstdial.hh			\
	bsteventroll.hh			\
	bsteventrollctrl.hh		\
	bstfiledialog.hh		\
	bstgconfig.hh			\
	bstgrowbar.hh			\
	bstitemseqdialog.hh		\
	bstitemview.hh			\
	bstkeybindings.hh		\
	bstknob.hh			\
	bstlogadjustment.hh		\
	bstmenus.hh			\
	bstmsgabsorb.hh			\
	bstparam.hh			\
	bstparamview.hh			\
	bstpartdialog.hh		\
	bstpartview.hh			\
	bstpatterncolumns.hh		\
	bstpatternctrl.hh		\
	bstpatternview.hh		\
	bstpianoroll.hh			\
	bstpianorollctrl.hh		\
	bstplayback.hh			\
	bstpreferences.hh		\
	bstprofiler.hh			\
	bstprojectctrl.hh		\
	bstqsampler.hh			\
	bstsampleeditor.hh		\
	bstscrollgraph.hh		\
	bstsegment.hh			\
	bstsequence.hh			\
	bstservermonitor.hh		\
	bstskinconfig.hh		\
	bstsnetrouter.hh		\
	bstsnifferscope.hh		\
	bstsoundfontpresetview.hh	\
	bstsoundfontview.hh		\
	bstsplash.hh			\
	bstsupershell.hh		\
	bsttrackroll.hh			\
	bsttrackrollctrl.hh		\
	bsttracksynthdialog.hh		\
	bsttrackview.hh			\
	bsttreestores.hh		\
	bstusermessage.hh		\
	bstutils.hh			\
	bstwaveeditor.hh		\
	bstwaveview.hh			\
	bstxframe.hh			\
	bstxkb.hh			\
	bstzoomedwindow.hh		\
)
# BEAST sources to build the program from
bst_cc_sources = $(strip \
	bstapp.cc			\
	bstasciipixbuf.cc		\
	bstauxdialogs.cc		\
	bstbseutils.cc			\
	bstbuseditor.cc			\
	bstbusmixer.cc			\
	bstbusview.cc			\
	bstcanvaslink.cc		\
	bstcanvassource.cc		\
	bstcluehunter.cc		\
	bstdbmeter.cc			\
	bstdial.cc			\
	bsteventroll.cc			\
	bsteventrollctrl.cc		\
	bstfiledialog.cc		\
	bstgconfig.cc			\
	bstgrowbar.cc			\
	bstitemseqdialog.cc		\
	bstitemview.cc			\
	bstkeybindings.cc		\
	bstknob.cc			\
	bstlogadjustment.cc		\
	bstmenus.cc			\
	bstmsgabsorb.cc			\
	bstparam.cc			\
	bstparamview.cc			\
	bstpartdialog.cc		\
	bstpartview.cc			\
	bstpatterncolumns.cc		\
	bstpatternctrl.cc		\
	bstpatternview.cc		\
	bstpianoroll.cc			\
	bstpianorollctrl.cc		\
	bstplayback.cc			\
	bstpreferences.cc		\
	bstprofiler.cc			\
	bstprojectctrl.cc		\
	bstqsampler.cc			\
	bstsampleeditor.cc		\
	bstscrollgraph.cc		\
	bstsegment.cc			\
	bstsequence.cc			\
	bstservermonitor.cc		\
	bstskinconfig.cc		\
	bstsnetrouter.cc		\
	bstsnifferscope.cc		\
	bstsoundfontpresetview.cc	\
	bstsoundfontview.cc		\
	bstsplash.cc			\
	bstsupershell.cc		\
	bsttrackroll.cc			\
	bsttrackrollctrl.cc		\
	bsttracksynthdialog.cc		\
	bsttrackview.cc			\
	bsttreestores.cc		\
	bstusermessage.cc		\
	bstutils.cc			\
	bstwaveeditor.cc		\
	bstwaveview.cc			\
	bstxframe.cc			\
	bstxkb.cc			\
	bstzoomedwindow.cc		\
)
# BEAST sources that get included (don't have own .lo rules)
beast_extra_files = $(strip 		\
	bstmarshal.list			\
	bstparam-choice.cc		\
	bstparam-color-spinner.cc	\
	bstparam-item-seq.cc		\
	bstparam-note-sequence.cc	\
	bstparam-note-spinner.cc	\
	bstparam-proxy.cc		\
	bstparam-scale.cc		\
	bstparam-searchpath.cc		\
	bstparam-time.cc		\
	bstparam-automation.cc		\
)

doc_sources = $(beast_headers) $(bst_cc_sources) $(beast_extra_files)

# == doc_sources.lst ==
doc_sources.lst: Makefile
	$(AM_V_GEN)
	$(Q) echo $(patsubst %, beast-gtk/%, $(doc_sources)) > $@
CLEANFILES += doc_sources.lst


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
	glib-genmarshal --prefix bst_marshal $(srcdir)/bstmarshal.list --header > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bstmarshal.cc
GENERATED_CLEANFILES += bstmarshal.cc
$(srcdir)/bstutils.cc: bstmarshal.cc
bstmarshal.cc: bstmarshal.list
	glib-genmarshal --prefix bst_marshal $(srcdir)/bstmarshal.list --body > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)


# == bstapi.idl ==
AUXTYPES_PY = $(topdir)/bse/AuxTypes.py
%_interfaces.hh %_interfaces.cc %_handles.hh %_handles.cc .%.stamp: %.idl $(AIDACC) $(AIDACC_CXXSTUB_PY) $(AUXTYPES_PY)
	$(QGEN) # MAKE(1) supports parallel execution of a multiple-outputs-rule *only* for pattern rules (with '%')
	$(Q) $(AIDACC) -x CxxStub -x $(AUXTYPES_PY) -G strip-path=$(topdir)/ $<
	$(Q) sed -i -e '1i#define _(x) x' bstapi_interfaces.cc && sed -i -e '1i#undef _' bstapi_interfaces.cc
	$(Q) touch .bstapi.stamp # memorize completion without errors
CLEANFILES += bstapi_interfaces.hh bstapi_interfaces.cc bstapi_handles.hh bstapi_handles.cc .bstapi.stamp
$(srcdir)/bstutils.cc: bstapi_interfaces.cc
$(bst_cc_sources:%=$(srcdir)/%): bstapi_interfaces.hh .bstapi.stamp

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
beastbin_PROGRAMS = beast-@MAJOR@.@MINOR@.@MICRO@
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
