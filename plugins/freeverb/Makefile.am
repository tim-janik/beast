# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN="\"$(strip $(basename $(<F)))\"" -DBSE_COMPILATION
AM_CXXFLAGS += $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE

# install plugins under:
plugindir = $(bselibdir)/plugins

freeverb_la_SOURCES = allpass.cpp comb.cpp revmodel.cpp bsefreeverbcpp.cpp bsefreeverb.cc
freeverb_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
freeverb_la_LIBADD  = ../../bse/libbse-@MAJOR@.la $(SFI_LIBS)

plugin_LTLIBRARIES = freeverb.la
