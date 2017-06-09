# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = gui

AM_CPPFLAGS += -I$(top_srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN="\"$(strip $(basename $(<F)))\""
AM_CXXFLAGS += $(SPECTMORPH_CFLAGS) $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE -DQT_NO_EMIT

# install plugins under:
plugindir = $(bselibdir)/plugins

## SpectMorph OSC
spectmorphosc_la_SOURCES = spectmorphosc.cc
spectmorphosc_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
spectmorphosc_la_LIBADD  = ../../bse/libbse-@MAJOR@.la $(LIBBSE_LIBS) $(SPECTMORPH_LIBS)
plugin_LTLIBRARIES = spectmorphosc.la

EXTRA_DIST += spectmorphosc.idl

# setup generation of C++ plugins from idl files
SFIDL = ../../sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)
%.genidl.hh: %.idl # $(SFIDL)
	$(SFIDL) --plugin  --macro $(<F) $(SFIDL_INC) -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )

$(srcdir)/spectmorphosc.cc: spectmorphosc.genidl.hh

CLEANFILES += spectmorphosc.genidl.hh
