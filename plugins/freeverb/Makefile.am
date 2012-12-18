# Freeverb Makefile
# Copyright (C) 2002 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I. $(BSE_CFLAGS) -DG_LOG_DOMAIN="\"$(strip $(basename $(<F)))\""

# install plugins under:
plugindir = $(bseplugindir)

freeverb_la_SOURCES = allpass.cpp comb.cpp revmodel.cpp bsefreeverbcpp.cpp bsefreeverb.cc
freeverb_la_LDFLAGS = -module -avoid-version
freeverb_la_LIBADD  = $(top_builddir)/bse/libbse.la $(BSE_LIBS) -lm

plugin_LTLIBRARIES = freeverb.la

EXTRA_DIST += $(strip \
	bsefreeverb.h \
	bsefreeverbcpp.h \
	denormals.h \
	tuning.h \
	allpass.hpp \
	comb.hpp \
	revmodel.hpp \
        readme.txt \
)
