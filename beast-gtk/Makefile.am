# BEAST - Bedevilled Audio System
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BEAST
 
SUBDIRS = icons

INCLUDES = -I$(top_srcdir) $(BEAST_CFLAGS) -I. \
           -DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN # -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_COMPAT_H


#
# programs to compile and install
#
bin_PROGRAMS = beast

# we specify the logging domain here
DEFS += $(strip \
	-DG_LOG_DOMAIN=\"BEAST\" \
)

# special profiling hooks
PROFILE_SOURCE = # profile.c
CFLAGS += # $(subst profile.c, -pg -a, $(findstring profile.c, $(<F)))
LIBS += # -lefence # -pg 

# link programs against BSE and other neccessary libraries
progs_LDADD = \
	$(top_builddir)/bse/libbse.la \
	$(top_builddir)/bsw/libbsw.la \
	$(LDFLAGS) $(BEAST_LIBS)
beast_LDADD = $(progs_LDADD)


#
# setup source file variables
#
# BEAST header files that don't get installed
beast_headers = $(strip \
	bstapp.h \
	bstasciipixbuf.h \
	bstcanvaslink.h \
	bstcanvassource.h \
	bstcontrollers.h \
	bstdefs.h \
	bstdial.h \
	bstdialog.h \
	bsteffectview.h \
	bstfiledialog.h \
	bstfreeradiobutton.h \
	bstgconfig.h \
	bstinstrumentview.h \
	bstitemview.h \
	bstkeytables.h \
	bstknob.h \
	bstlogadjustment.h \
	bstmarshal.h \
	bstmenus.h \
	bstparam.h \
	bstparamview.h \
	bstpatterndialog.h \
	bstpatterneditor.h \
	bstpatternview.h \
	bstpianoroll.h \
	bstplayback.h \
	bstplaylist.h \
	bstpreferences.h \
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
	bststatusbar.h \
	bstsupershell.h \
	bstutils.h \
	bstwavedialog.h \
	bstwaveeditor.h \
	bstwavereposhell.h \
	bstwaveview.h \
	bstxframe.h \
	bstxkb.h \
	bstzoomedwindow.h \
	glewidgets.h \
)
# BEAST c sources to build the program from
beast_sources = $(strip \
	bstapp.c \
	bstasciipixbuf.c \
	bstcanvaslink.c \
	bstcanvassource.c \
	bstcontrollers.c \
	bstdial.c \
	bstdialog.c \
	bsteffectview.c \
	bstfiledialog.c \
	bstfreeradiobutton.c \
	bstgconfig.c \
	bstinstrumentview.c \
	bstitemview.c \
	bstkeytables.c \
	bstknob.c \
	bstlogadjustment.c \
	bstmain.c \
	bstmenus.c \
	bstparam.c \
	bstparamview.c \
	bstpatterndialog.c \
	bstpatterneditor.c \
	bstpatternview.c \
	bstpianoroll.c \
	bstplayback.c \
	bstplaylist.c \
	bstpreferences.c \
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
	bststatusbar.c \
	bstsupershell.c \
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
)
# BEAST sources that get included (don't have own .lo rules)
beast_extra_sources = $(strip \
	bstpatterneditorselection.c \
	bstrackeditor-covers.c \
	bstkeytable-base.c \
	bstkeytable-us.c \
	bstkeytable-de.c \
	bstdraggroup.c \
	bstdragpattern.c \
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
beast_SOURCES = $(beast_sources)
noinst_PROGRAMS = bststest
bststest_SOURCES = bststest.c bstutils.c bstqsampler.c
bststest_LDADD = $(progs_LDADD)

EXTRA_DIST += $(beast_headers) $(beast_extra_sources)
MAINTAINERCLEANFILES += $(beast_built_sources)
EXTRA_DIST += $(beast_built_sources)

#
# rules to generate built sources
#
gen_source = gen-source.tmp
CLEANFILES += $(gen_source)
COPYING: $(beast_built_sources)
$(OBJECTS): COPYING # this is our oldest-source-stamp


#
# private (birnet) dependancy to make TAGS file after each build
#
all: @MISSING_ETAGS@ @BIRNET_Y@ # TAGS


#
# auxillary files
#
EXTRA_DIST += \
	TODO
