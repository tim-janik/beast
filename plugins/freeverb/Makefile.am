# Freeverb Makefile
#

INCLUDES += -I$(top_srcdir) -I. $(BSE_CFLAGS) -DG_LOG_DOMAIN=BSE_PLUGIN_NAME
INCLUDES += -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\""

# install plugins under:
plugindir = $(bseplugindir)

# all plugins want to link against:
plugin_libs = $(BSE_LIBS) -lm

freeverb_la_SOURCES = allpass.cpp comb.cpp revmodel.cpp bsefreeverbcpp.cpp bsefreeverb.c
freeverb_la_LDFLAGS = -module -avoid-version
freeverb_la_LIBADD  = $(plugin_libs)

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
