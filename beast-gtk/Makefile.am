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
	$(top_builddir)/bse/libbse.la \
	$(top_builddir)/bsw/libbsw.la \
	./gxk/libgxk.a \
	$(LDFLAGS) $(BEAST_LIBS) \
)

#
# setup source file variables
#
# BEAST header files that don't get installed
beast_headers = $(strip \
	bstapp.h \
	bstasciipixbuf.h \
	bstcanvaslink.h \
	bstcanvassource.h \
	bstcluehunter.h \
	bstcontrollers.h \
	bstdefs.h \
	bstdial.h \
	bstdragutils.h \
	bstfiledialog.h \
	bstfreeradiobutton.h \
	bstgconfig.h \
	bstitemview.h \
	bstkeytables.h \
	bstknob.h \
	bstlogadjustment.h \
	bstmarshal.h \
	bstmenus.h \
	bstparam.h \
	bstparamview.h \
	bstpartdialog.h \
	bstpartview.h \
	bstpianoroll.h \
	bstpianorollctrl.h \
	bstplayback.h \
	bstpreferences.h \
	bstprocbrowser.h \
	bstprocedure.h \
	bstqsampler.h \
	bstrackeditor.h \
	bstrackitem.h \
	bstracktable.h \
	bstradiotools.h \
	bstsampleeditor.h \
	bstsequence.h \
	bstservermonitor.h \
	bstsnetshell.h \
	bstsnetrouter.h \
	bstsongshell.h \
	bstsplash.h \
	bsttexttools.h \
	bsttoolbar.h \
	bsttrackview.h \
	bstsupershell.h \
	bstusermessage.h \
	bstutils.h \
	bstwavedialog.h \
	bstwaveeditor.h \
	bstwavereposhell.h \
	bstwaveview.h \
	bstxframe.h \
	bstxkb.h \
	bstzoomedwindow.h \
	glewidgets.h \
	gtklistwrapper.h \
)
# FIXME: remove extra dist for bstpatterneditor.h which is required by keytable legacy code
EXTRA_DIST += bstpatterneditor.h
# BEAST c sources to build the program from
beast_sources = $(strip \
	bstapp.c \
	bstasciipixbuf.c \
	bstcanvaslink.c \
	bstcanvassource.c \
	bstcluehunter.c \
	bstcontrollers.c \
	bstdial.c \
	bstdragutils.c \
	bstfiledialog.c \
	bstfreeradiobutton.c \
	bstgconfig.c \
	bstitemview.c \
	bstkeytables.c \
	bstknob.c \
	bstlogadjustment.c \
	bstmenus.c \
	bstparam.c \
	bstparamview.c \
	bstpartdialog.c \
	bstpartview.c \
	bstpianoroll.c \
	bstpianorollctrl.c \
	bstplayback.c \
	bstpreferences.c \
	bstprocbrowser.c \
	bstprocedure.c \
	bstqsampler.c \
	bstradiotools.c \
	bstrackeditor.c \
	bstrackitem.c \
	bstracktable.c \
	bstsampleeditor.c \
	bstsequence.c \
	bstservermonitor.c \
	bstsnetshell.c \
	bstsnetrouter.c \
	bstsongshell.c \
	bstsplash.c \
	bsttexttools.c \
	bsttoolbar.c \
	bsttrackview.c \
	bstsupershell.c \
	bstusermessage.c \
	bstutils.c \
	bstwavedialog.c \
	bstwaveeditor.c \
	bstwavereposhell.c \
	bstwaveview.c \
	bstxframe.c \
	bstxkb.c \
	bstzoomedwindow.c \
	$(PROFILE_SOURCE) \
	glewidgets.c \
	gtklistwrapper.c \
)
# BEAST sources that get included (don't have own .lo rules)
beast_extra_sources = $(strip \
	bstrackeditor-covers.c \
	bstkeytable-base.c \
	bstkeytable-us.c \
	bstkeytable-de.c \
	bstmarshal.c \
	bstmarshal.list \
	bstenum_arrays.stamp \
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
$(srcdir)/bstgentypes.h: bstenum_arrays.stamp
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "#define BST_TYPE_@ENUMSHORT@\t    (bst__type_id__@EnumName@)\n" \
	  --eprod "extern GType bst__type_id__@EnumName@;" \
	    $(beast_headers) > xgen-bgth \
	&& cp xgen-bgth $@ \
	&& rm -f xgen-bgth
$(srcdir)/bstgentypes.c: bstenum_arrays.stamp
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --eprod "\nGType bst__type_id__@EnumName@ = 0;" \
	    $(beast_headers) > xgen-bgtc \
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

#
# setup BEAST sources and their dependancies
#
bin_PROGRAMS = beast
beast_SOURCES = bstmain.c $(beast_sources)
beast_LDADD = $(progs_LDADD)
noinst_PROGRAMS = bststest tsmview
bststest_SOURCES = bststest.c bstutils.c bstqsampler.c
bststest_LDADD = $(progs_LDADD)
tsmview_SOURCES = tsmview.c $(beast_sources)
tsmview_LDADD = $(progs_LDADD)

EXTRA_DIST += $(beast_headers) $(beast_extra_sources)
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
