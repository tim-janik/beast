# GXK - Gtk+ Extension Kit
# Copyright (C) 2002 Tim Janik
#
## Makefile.am for GXK

SUBDIRS =

INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/beast-gtk $(GXK_CFLAGS) # -I.. for <gxk/*.h> includes

DEFS += $(strip \
	-DGTK_DISABLE_COMPAT_H 	\
	-DGXK_COMPILATION	\
	-DG_LOG_DOMAIN=\"GXK\" 	\
)
# -DGTK_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN

noinst_LIBRARIES = libgxk.a

gxk_headers = $(strip \
	gxkled.h	gxkcanvas.h		gxkdialog.h	gxkauxwidgets.h \
	gxkstatusbar.h	gxkstock.h		gxktexttools.h	gxklistwrapper.h \
	gxkrackeditor.h	gxkutils.h		gxkpolygon.h	gxkcellrendererpopup.h \
	gxkgadget.h	gxkaction.h		gxkglobals.h	gxkgadgetfactory.h \
	glewidgets.h	gxkracktable.h		gxkrackitem.h	gxksimplelabel.h \
	gxkparam.h	gxkscrollcanvas.h	gxkimagecache.h	gxklogadjustment.h \
	gxk.h \
)
gxk_c_sources = $(strip \
	gxkled.c	gxkcanvas.c		gxkdialog.c	gxkauxwidgets.c \
	gxkstatusbar.c	gxkstock.c		gxktexttools.c	gxklistwrapper.c \
	gxkrackeditor.c	gxkutils.c		gxkpolygon.c	gxkcellrendererpopup.c \
	gxkgadget.c	gxkaction.c		gxkglobals.c	gxkgadgetfactory.c \
	glewidgets.c	gxkracktable.c		gxkrackitem.c	gxksimplelabel.c \
	gxkparam.c	gxkscrollcanvas.c	gxkimagecache.c	gxklogadjustment.c \
)
gxk_extra_sources = $(strip \
	gxkparam-entry.c	gxkparam-label.c	\
	gxkparam-scale.c	gxkparam-spinner.c	\
	gxkparam-toggle.c	\
	gxkrackcovers.c		\
	gxkmarshal.list		\
)
gxk_built_sources = $(strip \
	gxkmarshal.h	gxkmarshal.c	gxkgentypes.h	gxkgentypes.c \
)

GLIB_MKENUMS = glib-mkenums
temp_files = xgen-gmh xgen-gmc xgen-ggth xgen-ggtc
stamp_files = gxkgentypes.stamp

$(gxk_c_sources): $(gxk_built_sources)

$(srcdir)/gxkmarshal.h:	gxkmarshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=gxk_marshal gxkmarshal.list --header >> xgen-gmh \
	&& cp xgen-gmh gxkmarshal.h \
	&& rm -f xgen-gmh
$(srcdir)/gxkmarshal.c: gxkmarshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=gxk_marshal gxkmarshal.list --body >> xgen-gmc \
	&& cp xgen-gmc gxkmarshal.c \
	&& rm -f xgen-gmc
$(srcdir)/gxkgentypes.h: gxkgentypes.stamp ;
$(srcdir)/gxkgentypes.stamp: gxkgentypes.c
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "#define GXK_TYPE_@ENUMSHORT@\t    (gxk__type_id__@EnumName@)\n" \
	  --eprod "extern GType gxk__type_id__@EnumName@;" \
	    $(gxk_headers) > xgen-ggth \
	&& (cmp -s xgen-ggth gxkgentypes.h || cp xgen-ggth gxkgentypes.h) \
	&& rm -f xgen-ggth \
	&& echo timestamp > $(@F)
$(srcdir)/gxkgentypes.c: $(gxk_headers)
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	  --vhead "\nGType gxk__type_id__@EnumName@ = 0;" \
	  --vhead "\nstatic G@Type@Value @enum_name@_values[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "  { 0, NULL, NULL }\n};\n" \
	    $(gxk_headers) > xgen-ggtc \
	&& $(GLIB_MKENUMS) \
	  --fhead "static const GxkTypeGenerated generated_type_entries[] = {" \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &gxk__type_id__@EnumName@, @enum_name@_values }," \
	  --ftail "\n};" \
	    $(gxk_headers) >> xgen-ggtc \
        && cp xgen-ggtc $@ \
	&& rm -f xgen-ggtc

libgxk_a_SOURCES = $(gxk_c_sources)

progs_LDADD = $(strip \
        libgxk.a \
        $(top_builddir)/sfi/libsfi.la \
        $(LDFLAGS) $(BEAST_LIBS) \
)

noinst_PROGRAMS = gxktest
gxktest_SOURCES = gxktest.c
gxktest_LDADD = $(progs_LDADD)

EXTRA_DIST += $(gxk_headers) $(gxk_extra_sources) $(gxk_built_sources)
MAINTAINERCLEANFILES += $(gxk_built_sources) $(stamp_files)
CLEANFILES += $(temp_files)
