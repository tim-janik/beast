# BEAST - Bedevilled Audio System
# Copyright (C) 1998-2002 Tim Janik
#
## Makefile.am for BEAST
 
SUBDIRS = gxk icons images

INCLUDES = $(strip \
	-I$(top_srcdir)	\
	-I. \
	$(BEAST_CFLAGS) \
	-DG_DISABLE_CONST_RETURNS \
	-DGTK_ENABLE_BROKEN \
)
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
	bstsampleeditor.h	bstsequence.h	bstservermonitor.h	bstsnetshell.h	\
	bsttrackroll.h		bstcluehunter.h	bstprojectctrl.h	bstdial.h	\
	bstfreeradiobutton.h	bstdragutils.h	bstfiledialog.h		bstgconfig.h	\
	bstlogadjustment.h	bstitemview.h	bstmarker.h		bstknob.h	\
	bstparamview.h		bstmenus.h	bstparam.h		bstpartdialog.h	\
	bstpianorollctrl.h	bstpartview.h	bstpianoroll.h		bstplayback.h	\
	bstprocbrowser.h	bstqsampler.h	bstpreferences.h	bstprocedure.h	\
	bstrackeditor.h		bstrackitem.h	bstracktable.h		bstradiotools.h	\
	bstsnetrouter.h		bstsplash.h	bsttrackview.h		bstsupershell.h	\
	bstusermessage.h	bstutils.h	bstwavedialog.h		bstwaveeditor.h	\
	bstwavereposhell.h	bstwaveview.h	bstsongshell.h		bstxkb.h	\
	bstzoomedwindow.h	glewidgets.h	bsttrackrollctrl.h	bstxframe.h	\
	bstcatalog.h		bstdefs.h bstmarshal.h \
)
# BEAST c sources to build the program from
beast_sources = $(strip \
	bstcanvassource.c	bstapp.c	bstasciipixbuf.c	bstcanvaslink.c	\
	bstsampleeditor.c	bstsequence.c	bstservermonitor.c	bstsnetshell.c	\
	bsttrackroll.c		bstcluehunter.c	bstprojectctrl.c	bstdial.c	\
	bstfreeradiobutton.c	bstdragutils.c	bstfiledialog.c		bstgconfig.c	\
	bstlogadjustment.c	bstitemview.c	bstmarker.c		bstknob.c	\
	bstparamview.c		bstmenus.c	bstparam.c		bstpartdialog.c	\
	bstpianorollctrl.c	bstpartview.c	bstpianoroll.c		bstplayback.c	\
	bstprocbrowser.c	bstqsampler.c	bstpreferences.c	bstprocedure.c	\
	bstrackeditor.c		bstrackitem.c	bstracktable.c		bstradiotools.c	\
	bstsnetrouter.c		bstsplash.c	bsttrackview.c		bstsupershell.c	\
	bstusermessage.c	bstutils.c	bstwavedialog.c		bstwaveeditor.c	\
	bstwavereposhell.c	bstwaveview.c	bstsongshell.c		bstxkb.c	\
	bstzoomedwindow.c	glewidgets.c	bsttrackrollctrl.c	bstxframe.c	\
	bstcatalog.c		$(PROFILE_SOURCE) \
)
# BEAST sources that get included (don't have own .lo rules)
beast_extra_sources = $(strip \
	bstparam-label.c	bstmarshal.c	 bstparam-toggle.c	bstparam-spinner.c \
	bstrackeditor-covers.c	bstparam-scale.c bstparam-choice.c	bstparam-note-sequence.c \
	bstparam-entry.c \
)
beast_extra_files = $(strip \
	catalog-en.c	bstmarshal.list		bstenum_arrays.stamp	stamp-ckeycheck \
	bstdebugkeys.defs \
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
$(srcdir)/bstenum_arrays.c: $(beast_headers)
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	  --vhead "/* @EnumName@\n */\n" \
	  --vhead "static G@Type@Value @enum_name@_values[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "  { 0, NULL, NULL }\n};\n" \
	    $(beast_headers) > xgen-beac \
	&& (cmp -s xgen-beac $@ || touch bstenum_arrays.stamp) \
	&& cp xgen-beac $@ \
	&& rm -f xgen-beac
$(srcdir)/bstenum_arrays.stamp: bstenum_arrays.c
	@ [ -f $@ ] || touch $@
$(srcdir)/bstgentypes.h: bstenum_arrays.stamp bstrecords.sfidl
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "#define BST_TYPE_@ENUMSHORT@\t    (bst__type_id__@EnumName@)\n" \
	  --eprod "extern GType bst__type_id__@EnumName@;" \
	    $(beast_headers) > xgen-bgth \
	&& $(SFIDL) --header --implementation bstrecords.sfidl >> xgen-bgth \
	&& cp xgen-bgth $@ \
	&& rm -f xgen-bgth
$(srcdir)/bstgentypes.c: bstenum_arrays.stamp bstrecords.sfidl
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --eprod "\nGType bst__type_id__@EnumName@ = 0;" \
	    $(beast_headers) > xgen-bgtc \
	&& $(SFIDL) --source --implementation --init _bst_init_idl bstrecords.sfidl >> xgen-bgtc \
        && cp xgen-bgtc $@ \
	&& rm -f xgen-bgtc
$(srcdir)/bstenum_list.c: bstenum_arrays.stamp
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &bst__type_id__@EnumName@, @enum_name@_values }," \
	    $(beast_headers) > xgen-belc \
	&& cp xgen-belc $@ \
	&& rm -f xgen-belc
$(srcdir)/bstmarshal.h: bstmarshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=bst_marshal bstmarshal.list --header >> xgen-bmh \
	&& cp xgen-bmh bstmarshal.h \
	&& rm -f xgen-bmh
$(srcdir)/bstmarshal.c: bstmarshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=bst_marshal bstmarshal.list --body >> xgen-bmc \
	&& cp xgen-bmc bstmarshal.c \
	&& rm -f xgen-bmc

SCANKEYS=$(top_builddir)/beast-gtk/scankeys
stamp-ckeycheck: scankeys $(beast_sources) $(beast_extra_sources)
	cd $(srcdir) \
	&& $(SCANKEYS) --count --key CKEY --skipkeys catalog-en.c \
	     --tmplstart '#error MISSING DOC KEYS:\n' \
	     --tmplkey '  { CKEY (%s),\t\t"Name",\n  BST_STOCK_,\t\t\t\t"",\n    "tip", NULL,\n  },\n' \
	     --needkeys $(beast_sources) $(beast_extra_sources)
	touch $@
beast_DEPENDENCIES += stamp-ckeycheck

debug_key_globs = $(top_srcdir)/beast-gtk/*.[hc] $(top_srcdir)/bse/*.[hc] $(top_srcdir)/sfi/*.[hc]
bstdebugkeys.defs: scankeys $(debug_key_globs)
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
noinst_PROGRAMS = bststest tsmview scankeys
bststest_SOURCES = bststest.c bstutils.c bstqsampler.c
bststest_LDADD = $(progs_LDADD)
tsmview_SOURCES = tsmview.c
tsmview_LDADD = $(progs_LDADD)

# scankeys
scankeys_SOURCES = scankeys.c
scankeys_LDADD = $(progs_LDADD)


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
all: @MISSING_ETAGS@ @BIRNET_Y@ # TAGS


#
# auxillary files
#
EXTRA_DIST += \
	TODO
