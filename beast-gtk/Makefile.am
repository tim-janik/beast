# BEAST - Bedevilled Audio System
# Copyright (C) 1998-2002 Tim Janik
#
## Makefile.am for BEAST
 
SUBDIRS = gxk dialogs icons

INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/beast-gtk $(BEAST_CFLAGS) -DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN
# -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_COMPAT_H

# we specify the logging domain here
DEFS += $(strip \
	-DG_LOG_DOMAIN=\"BEAST\" \
)

# special profiling hooks
PROFILE_SOURCE = # profile.c
CFLAGS += # $(subst profile.c, -pg -a, $(findstring profile.c, $(<F)))
LIBS += # -lefence # -pg 

# link programs against BSE and other neccessary libraries
progs_LDADD = $(strip \
	./gxk/libgxk.a \
	$(top_builddir)/bsw/libbsw.la \
	$(top_builddir)/bse/libbse.la \
	$(top_builddir)/sfi/libsfi.la \
	$(LDFLAGS) $(BEAST_LIBS) \
)

#
# setup source file variables
#
# BEAST header files that don't get installed
beast_headers = $(strip \
	bstcanvassource.h	bstapp.h	bstasciipixbuf.h	bstcanvaslink.h	\
	bstsampleeditor.h	bstsequence.h	bstservermonitor.h	\
	bsttrackroll.h		bstcluehunter.h	bstprojectctrl.h	bstxxutils.h	\
	bstsnifferscope.h	bstdragutils.h	bstfiledialog.h		bstgconfig.h	\
	bstlogadjustment.h	bstitemview.h	bstmarker.h		bstknob.h	\
	bstparamview.h		bstmenus.h	bstparam.h		bstpartdialog.h	\
	bstpianorollctrl.h	bstpartview.h	bstpianoroll.h		bstplayback.h	\
	bstprocbrowser.h	bstqsampler.h	bstpreferences.h	bstprocedure.h	\
	bstrackeditor.h		bstrackitem.h	bstracktable.h		\
	bstsnetrouter.h		bstsplash.h	bsttrackview.h		bstsupershell.h	\
	bstusermessage.h	bstdial.h	bsttracksynthdialog.h	bstwaveeditor.h	\
				bstwaveview.h				bstxkb.h	\
	bstzoomedwindow.h			bsttrackrollctrl.h	bstxframe.h	\
				bstrackview.h	bsttreestores.h		bstutils.h	\
	bstauxdialogs.h		bstsegment.h	bsteventrollctrl.h	bsteventroll.h	\
	bstmarshal.h bstdefs.h \
)
# BEAST c sources to build the program from
beast_sources = $(strip \
	bstcanvassource.c	bstapp.c	bstasciipixbuf.c	bstcanvaslink.c	\
	bstsampleeditor.c	bstsequence.c	bstservermonitor.c	\
	bsttrackroll.c		bstcluehunter.c	bstprojectctrl.c	bstxxutils.cc	\
	bstsnifferscope.c	bstdragutils.c	bstfiledialog.c		bstgconfig.c	\
	bstlogadjustment.c	bstitemview.c	bstmarker.c		bstknob.c	\
	bstparamview.c		bstmenus.c	bstparam.c		bstpartdialog.c	\
	bstpianorollctrl.c	bstpartview.c	bstpianoroll.c		bstplayback.c	\
	bstprocbrowser.c	bstqsampler.c	bstpreferences.c	bstprocedure.c	\
	bstrackeditor.c		bstrackitem.c	bstracktable.c		\
	bstsnetrouter.c		bstsplash.c	bsttrackview.c		bstsupershell.c	\
	bstusermessage.c	bstdial.c	bsttracksynthdialog.c	bstwaveeditor.c	\
				bstwaveview.c				bstxkb.c	\
	bstzoomedwindow.c			bsttrackrollctrl.c	bstxframe.c	\
				bstrackview.c	bsttreestores.c		bstutils.c	\
	bstauxdialogs.c		bstsegment.c	bsteventrollctrl.c	bsteventroll.c	\
	$(PROFILE_SOURCE) \
)
# BEAST sources that get included (don't have own .lo rules)
beast_extra_sources = $(strip \
	bstparam-choice.c	bstparam-note-sequence.c	\
	bstparam-note-spinner.c	bstparam-proxy.c		\
	bstparam-scale.c	bstparam-searchpath.c		\
	bstparam-time.c		\
	bstrackeditor-covers.c	bstmarshal.c \
)
beast_extra_files = $(strip \
	bstmarshal.list		stamp-bstenum_arrays.c	\
	bstdebugkeys.defs	bstrecords.idl		\
)
# BEAST built sources that don't get installed
beast_built_sources = @STRIP_BEGIN@ \
	bstmarshal.h \
	bstmarshal.c \
	bstgentypes.h \
	bstgentypes.c \
	bstenum_arrays.c \
	bstenum_list.c \
@STRIP_END@
GLIB_MKENUMS = glib-mkenums

SFIDL = ../sfi/sfidl    # FIXME: cd $(srcdir) && $(SFIDL) doesn't work for builddir


# gen-sources
CLEANFILES += xgen-beac xgen-beac xgen-bgth xgen-bgtc xgen-belc xgen-bmh xgen-bmc xgen-bdkd
bstenum_arrays.c: $(beast_headers)
	(cd ${srcdir} \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	  --vhead "/* @EnumName@\n */\n" \
	  --vhead "static G@Type@Value @enum_name@_values[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "  { 0, NULL, NULL }\n};\n" \
	    $(beast_headers) ) > xgen-beac \
	&& (cmp -s xgen-beac $@ || touch stamp-bstenum_arrays.c) \
	&& cp xgen-beac $@ \
	&& rm -f xgen-beac
$(srcdir)/stamp-bstenum_arrays.c: bstenum_arrays.c
	@ [ -f $@ ] || touch $@
bstgentypes.h: stamp-bstenum_arrays.c ${srcdir}/bstrecords.idl $(SFIDL)
	(cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "#define BST_TYPE_@ENUMSHORT@\t    (bst__type_id__@EnumName@)\n" \
	  --eprod "extern GType bst__type_id__@EnumName@;" \
	    $(beast_headers) ) > xgen-bgth \
	&& $(SFIDL) --header --implementation ${srcdir}/bstrecords.idl >> xgen-bgth \
	&& cp xgen-bgth $@ \
	&& rm -f xgen-bgth
$(srcdir)/bstgentypes.c: stamp-bstenum_arrays.c ${srcdir}/bstrecords.idl $(SFIDL)
	(cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --eprod "\nGType bst__type_id__@EnumName@ = 0;" \
	    $(beast_headers) ) > xgen-bgtc \
	&& $(SFIDL) --source --implementation --init _bst_init_idl ${srcdir}/bstrecords.idl >> xgen-bgtc \
        && cp xgen-bgtc $@ \
	&& rm -f xgen-bgtc
$(srcdir)/bstenum_list.c: stamp-bstenum_arrays.c
	(cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &bst__type_id__@EnumName@, @enum_name@_values }," \
	    $(beast_headers) ) > xgen-belc \
	&& cp xgen-belc $@ \
	&& rm -f xgen-belc
$(srcdir)/bstmarshal.h: bstmarshal.list
	glib-genmarshal --prefix=bst_marshal $(srcdir)/bstmarshal.list --header > xgen-bmh \
	&& cp xgen-bmh bstmarshal.h \
	&& rm -f xgen-bmh
$(srcdir)/bstmarshal.c: bstmarshal.list
	glib-genmarshal --prefix=bst_marshal $(srcdir)/bstmarshal.list --body > xgen-bmc \
	&& cp xgen-bmc bstmarshal.c \
	&& rm -f xgen-bmc

SCANKEYS=$(TEXITHEQUE) scankeys --

debug_key_globs = $(top_srcdir)/beast-gtk/*.[hc] $(top_srcdir)/bse/*.[hc] $(top_srcdir)/sfi/*.[hc]
bstdebugkeys.defs: @DVLBUILD@ $(debug_key_globs)
	cd $(srcdir) \
	&& echo > xgen-bdkd \
	&& grep -E 'sfi_debug_(keyfunc|test_key)' $(debug_key_globs) \
	| $(SCANKEYS) --key sfi_debug_keyfunc --key sfi_debug_test_key \
	     --tmplstart '#define BST_LOG_SCAN_KEYS \\\n' \
	     --tmplkey   '  %s, \\\n' \
	     --tmplend   '\n' 	--needkeys - >> xgen-bdkd \
	&& cp xgen-bdkd $@ \
	&& rm -f xgen-bdkd
bstutils.c: bstdebugkeys.defs

#
# setup BEAST sources and their dependancies
#
bin_PROGRAMS = beast
beast_SOURCES = bstmain.c $(beast_sources)
beast_LDADD = $(progs_LDADD)
noinst_PROGRAMS = tsmview # bststest
# bststest_SOURCES = bststest.c bstutils.c bstqsampler.c cxxdummy.cc
# bststest_LDADD = $(progs_LDADD)
tsmview_SOURCES = tsmview.c cxxdummy.cc
tsmview_LDADD = $(progs_LDADD)


# extra deps
$(beast_SOURCES) $(bststest_SOURCES) $(tsmview_SOURCES): bstgentypes.h

EXTRA_DIST += $(beast_headers) $(beast_extra_sources) $(beast_extra_files)
MAINTAINERCLEANFILES += $(beast_built_sources)
EXTRA_DIST += $(beast_built_sources)

#
# rules to generate built sources
#
gen_source = gen-source.tmp
CLEANFILES += $(gen_source)
$(beast_sources): $(beast_built_sources)


#
# private (birnet) dependancy to make TAGS file after each build
#
all: # TAGS

