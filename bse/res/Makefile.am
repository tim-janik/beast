# BEAST - Better Audio System
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir)
AM_CXXFLAGS += $(BSE_CFLAGS) -DG_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS
RAPIDRES     = rapidres

RESOURCES = $(strip 		\
	gus-patch.bse		\
	adsr-wave-1.bse		\
	adsr-wave-2.bse		\
	plain-wave-1.bse	\
	plain-wave-2.bse	\
)

gen_sources = xgen-bzc xgen-bzh
CLEANFILES += $(gen_sources) bse-resources.cc
noinst_DATA = bse-resources.cc
EXTRA_DIST += $(RESOURCES)

bse-resources.cc: $(srcdir)/*.bse
	(cd $(srcdir) && $(RAPIDRES) $(RESOURCES) ) > xgen-bzc				\
	&& cp xgen-bzc $@								\
	&& rm -f xgen-bzc
