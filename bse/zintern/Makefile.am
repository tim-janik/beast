# BEAST - Better Audio System
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir)
AM_CXXFLAGS += $(BSE_CFLAGS) -DG_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS
RAPIDRES     = rapidres

ZFILE_DEFS = $(strip \
	$(srcdir)/gus-patch.bse		\
	$(srcdir)/adsr-wave-1.bse	\
	$(srcdir)/adsr-wave-2.bse	\
	$(srcdir)/plain-wave-1.bse	\
	$(srcdir)/plain-wave-2.bse	\
)

gen_sources = xgen-bzc xgen-bzh
CLEANFILES += $(gen_sources) bse-resources.cc
noinst_DATA = bse-resources.cc
EXTRA_DIST += $(filter %.bse, $(ZFILE_DEFS))

bse-resources.cc: $(srcdir)/*.bse
	$(RAPIDRES) $(ZFILE_DEFS) > xgen-bzc						\
	&& echo "static const BseZFile bse_zfiles[] = {" >> xgen-bzc			\
	&& for i in $(filter-out %.bse, $(ZFILE_DEFS)) ; do				\
		uname=$$(echo $$i | tr a-z- A-Z_);					\
		echo "  { \"$$i\", $$uname""_SIZE, "	>>xgen-bzc;			\
		echo "    $$uname""_DATA, G_N_ELEMENTS ($$uname""_DATA) }," >>xgen-bzc;	\
	done										\
	&& echo "};" >> xgen-bzc							\
	&& cp xgen-bzc $@								\
	&& rm -f xgen-bzc

# need -I$(top_srcdir) for <gsl/gsldefs.h>
# need -I. for "../topconfig.h"
