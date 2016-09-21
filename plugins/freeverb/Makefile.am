# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN="\"$(strip $(basename $(<F)))\""
AM_CXXFLAGS += $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE

# install plugins under:
plugindir = $(bselibdir)/plugins

freeverb_la_SOURCES = allpass.cpp comb.cpp revmodel.cpp bsefreeverbcpp.cpp bsefreeverb.cc
freeverb_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
freeverb_la_LIBADD  = $(top_builddir)/bse/libbse-@MAJOR@.la $(BSE_LIBS) -lm

plugin_LTLIBRARIES = freeverb.la

EXTRA_DIST += $(strip \
	bsefreeverb.hh \
	bsefreeverbcpp.hh \
	denormals.hh \
	tuning.hh \
	allpass.hpp \
	comb.hpp \
	revmodel.hpp \
        readme.txt \
)
