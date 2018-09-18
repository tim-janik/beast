# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS =

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/beast-gtk -I$(top_builddir)/beast-gtk -I$(srcdir) # -I.. for <gxk/*.h> includes
DEFS        += -DGTK_DISABLE_COMPAT_H -DGXK_COMPILATION -DG_LOG_DOMAIN=\"GXK\" # -DGTK_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN
AM_CXXFLAGS += $(GXK_CFLAGS) $(BSE_CFLAGS)
AM_CFLAGS   += $(GXK_CFLAGS)

noinst_LIBRARIES = libgxk.a

gxk_headers = $(strip		\
	glewidgets.h		\
	gxk.hh			\
	gxkaction.hh		\
	gxkassortment.hh	\
	gxkauxwidgets.hh	\
	gxkcanvas.hh		\
	gxkcellrendererpopup.hh	\
	gxkcompat.hh		\
	gxkdialog.hh		\
	gxkglobals.hh		\
	gxkimagecache.hh	\
	gxkled.hh		\
	gxklistwrapper.hh	\
	gxklogadjustment.hh	\
	gxkmenubutton.hh	\
	gxknotebook.hh		\
	gxkparam.hh		\
	gxkpolygon.hh		\
	gxkrackeditor.hh	\
	gxkrackitem.hh		\
	gxkracktable.hh		\
	gxkradget.hh		\
	gxkradgetfactory.hh	\
	gxkscrollcanvas.hh	\
	gxksimplelabel.hh	\
	gxkspline.hh		\
	gxkstatusbar.hh		\
	gxkstock.hh		\
	gxktexttools.hh		\
	gxkutils.hh		\
)
gxk_c_sources = $(strip		\
	glewidgets.c		\
	gxkaction.cc		\
	gxkassortment.cc	\
	gxkauxwidgets.cc	\
	gxkcanvas.cc		\
	gxkcellrendererpopup.cc \
	gxkcompat.cc		\
	gxkdialog.cc		\
	gxkglobals.cc		\
	gxkimagecache.cc	\
	gxkled.cc		\
	gxklistwrapper.cc	\
	gxklogadjustment.cc	\
	gxkmenubutton.cc	\
	gxknotebook.cc		\
	gxkparam.cc		\
	gxkpolygon.cc		\
	gxkrackeditor.cc	\
	gxkrackitem.cc		\
	gxkracktable.cc		\
	gxkradget.cc		\
	gxkradgetfactory.cc	\
	gxkscrollcanvas.cc	\
	gxksimplelabel.cc 	\
	gxkspline.cc		\
	gxkstatusbar.cc		\
	gxkstock.cc		\
	gxktexttools.cc		\
	gxkutils.cc		\
)
gxk_extra_sources = $(strip	\
	gxkmarshal.list		\
	gxkparam-entry.cc	\
	gxkparam-label.cc	\
	gxkparam-scale.cc	\
	gxkparam-spinner.cc	\
	gxkparam-toggle.cc	\
	gxkrackcovers.cc	\
)

doc_sources = $(gxk_headers) $(gxk_c_sources) $(gxk_extra_sources)

# == doc_sources.lst ==
doc_sources.lst: Makefile
	$(AM_V_GEN)
	$(Q) echo $(patsubst %, beast-gtk/gxk/%, $(doc_sources)) > $@
CLEANFILES += doc_sources.lst

GLIB_MKENUMS = glib-mkenums

CLEANSTAMPFILES =
if WITH_PERLRULE
CLEANFILES += $(CLEANSTAMPFILES)
else
MAINTAINERCLEANFILES += $(CLEANSTAMPFILES)
endif

# gxkgentypes.h
GENERATED_CLEANFILES += gxkgentypes.h
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
	  && glib-genmarshal --prefix gxk_marshal gxkmarshal.list --header \
	) > xgen-$(@F) \
	&& echo '#define gxk_marshal_VOID__POINTER      g_cclosure_marshal_VOID__POINTER'      >> xgen-$(@F) \
	&& echo '#define gxk_marshal_VOID__UINT_POINTER g_cclosure_marshal_VOID__UINT_POINTER' >> xgen-$(@F) \
	&& echo '#define gxk_marshal_VOID__BOXED        g_cclosure_marshal_VOID__BOXED'        >> xgen-$(@F) \
	&& echo '#define gxk_marshal_VOID__BOOLEAN      g_cclosure_marshal_VOID__BOOLEAN'      >> xgen-$(@F) \
	&& echo '#define gxk_marshal_VOID__STRING       g_cclosure_marshal_VOID__STRING'       >> xgen-$(@F) \
	&& echo '#define gxk_marshal_VOID__INT          g_cclosure_marshal_VOID__INT'          >> xgen-$(@F) \
	&& echo '#define gxk_marshal_VOID__UINT         g_cclosure_marshal_VOID__UINT'         >> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# gxkmarshal.cc
GENERATED_CLEANFILES += gxkmarshal.cc
$(libgxk_a_OBJECTS): gxkmarshal.cc
gxkmarshal.cc: gxkmarshal.list
	( \
	  cd $(srcdir) \
	  && glib-genmarshal --prefix gxk_marshal gxkmarshal.list --body \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)

libgxk_a_SOURCES = $(gxk_c_sources)

noinst_PROGRAMS = $(ALLTESTS)
progs_LDADD     = $(strip					\
	libgxk.a $(GXK_LIBS) 					\
	$(top_builddir)/bse/libbse-@MAJOR@.la $(BSE_LIBS)	\
	$(LIBZ_LIBS)						\
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
