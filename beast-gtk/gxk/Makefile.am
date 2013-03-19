# GXK - Gtk+ Extension Kit
include $(top_srcdir)/Makefile.decl

SUBDIRS =

INCLUDES    += -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/beast-gtk -I$(top_builddir)/beast-gtk -I$(srcdir) # -I.. for <gxk/*.h> includes
DEFS        += @DEFINE__FILE_DIR__@ -DGTK_DISABLE_COMPAT_H -DGXK_COMPILATION -DG_LOG_DOMAIN=\"GXK\" # -DGTK_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN
AM_CXXFLAGS  = $(GXK_CFLAGS) $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE
AM_CFLAGS    = $(GXK_CFLAGS)

noinst_LIBRARIES = libgxk.a

gxk_headers = $(strip \
	gxkled.hh	gxkcanvas.hh		gxkdialog.hh	gxkauxwidgets.hh \
	gxkstatusbar.hh	gxkstock.hh		gxktexttools.hh	gxklistwrapper.hh \
	gxkrackeditor.hh	gxkutils.hh		gxkpolygon.hh	gxkcellrendererpopup.hh \
	gxkradget.hh	gxkaction.hh		gxkglobals.hh	gxkradgetfactory.hh \
	glewidgets.h	gxkracktable.hh		gxkrackitem.hh	gxksimplelabel.hh \
	gxkparam.hh	gxkscrollcanvas.hh	gxkimagecache.hh	gxklogadjustment.hh \
	gxkmenubutton.hh	gxkspline.hh		gxkassortment.hh gxknotebook.hh \
	gxk.hh \
)
gxk_c_sources = $(strip \
	gxkled.cc	gxkcanvas.cc		gxkdialog.cc	gxkauxwidgets.cc \
	gxkstatusbar.cc	gxkstock.cc		gxktexttools.cc	gxklistwrapper.cc \
	gxkrackeditor.cc	gxkutils.cc		gxkpolygon.cc	gxkcellrendererpopup.cc \
	gxkradget.cc	gxkaction.cc		gxkglobals.cc	gxkradgetfactory.cc \
	glewidgets.c	gxkracktable.cc		gxkrackitem.cc	gxksimplelabel.cc \
	gxkparam.cc	gxkscrollcanvas.cc	gxkimagecache.cc	gxklogadjustment.cc \
	gxkmenubutton.cc	gxkspline.cc		gxkassortment.cc gxknotebook.cc \
)
gxk_extra_sources = $(strip \
	gxkparam-entry.cc	gxkparam-label.cc	\
	gxkparam-scale.cc	gxkparam-spinner.cc	\
	gxkparam-toggle.cc	\
	gxkrackcovers.cc		\
	gxkmarshal.list		\
)

EXTRA_DIST += $(gxk_headers) $(gxk_extra_sources)

GLIB_MKENUMS = glib-mkenums

CLEANSTAMPFILES =
if WITH_PERLRULE
CLEANFILES += $(CLEANSTAMPFILES)
else
MAINTAINERCLEANFILES += $(CLEANSTAMPFILES)
endif

# gxkgentypes.h
GENERATED_EXTRA_DIST += gxkgentypes.h
$(libgxk_a_OBJECTS): gxkgentypes.h
gxkgentypes.h: @PERLRULE@ stamp-gxkgentypes.h ;
CLEANSTAMPFILES += stamp-gxkgentypes.h
stamp-gxkgentypes.h: @PERLRULE@ $(gxk_headers)
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */" \
	    --eprod "#define GXK_TYPE_@ENUMSHORT@\t    (gxk__type_id__@EnumName@)\n" \
	    --eprod "extern GType gxk__type_id__@EnumName@;" \
	      $(gxk_headers) \
	) > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(srcdir)/gxkgentypes.h || cp xgen-$(@F) $(srcdir)/gxkgentypes.h) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)
# gxkgentypes.cc
GENERATED_CLEANFILES += gxkgentypes.cc
$(libgxk_a_OBJECTS): gxkgentypes.cc
gxkgentypes.cc: $(gxk_headers)
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	    --vhead "\nGType gxk__type_id__@EnumName@ = 0;" \
	    --vhead "\nstatic G@Type@Value @enum_name@_values[] = {" \
	    --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	    --vtail "  { 0, NULL, NULL }\n};\n" \
	      $(gxk_headers) \
	  && $(GLIB_MKENUMS) \
	    --fhead "static const GxkTypeGenerated generated_type_entries[] = {" \
	    --fprod "\n/* --- @filename@ --- */" \
	    --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &gxk__type_id__@EnumName@, @enum_name@_values }," \
	    --ftail "\n};" \
	      $(gxk_headers) \
	) > xgen-$(@F) \
        && cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# gxkmarshal.h
GENERATED_CLEANFILES += gxkmarshal.h
$(libgxk_a_OBJECTS): gxkmarshal.h
gxkmarshal.h: gxkmarshal.list
	( \
	  cd $(srcdir) \
	  && glib-genmarshal --prefix=gxk_marshal gxkmarshal.list --header \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# gxkmarshal.cc
GENERATED_CLEANFILES += gxkmarshal.cc
$(libgxk_a_OBJECTS): gxkmarshal.cc
gxkmarshal.cc: gxkmarshal.list
	( \
	  cd $(srcdir) \
	  && glib-genmarshal --prefix=gxk_marshal gxkmarshal.list --body \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)

libgxk_a_SOURCES = $(gxk_c_sources)

noinst_PROGRAMS = $(ALLTESTS)
progs_LDADD     = $(strip					\
	libgxk.a $(GXK_LIBS) 					\
	$(top_builddir)/sfi/libsfi.o $(SFI_LIBS) 		\
	$(RAPICORN_LIBS) 					\
	$(top_builddir)/birnet/libbirnet.o $(BIRNET_LIBS)	\
)


noinst_PROGRAMS += gxktest
gxktest_SOURCES  = gxktest.cc
gxktest_LDADD    = $(progs_LDADD)

noinst_PROGRAMS   += splinetest
splinetest_SOURCES = splinetest.cc
splinetest_LDADD   = $(progs_LDADD)


#
# convenience targets for generated source files
#
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(GENERATED) $(CLEANSTAMPFILES)
generated: clean-generated $(GENERATED)
