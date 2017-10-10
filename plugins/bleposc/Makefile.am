# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN="\"$(strip $(basename $(<F)))\"" -DBSE_PLUGIN_COMPILATION
AM_CXXFLAGS += $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE

# install plugins under:
plugindir = $(bselibdir)/plugins

## Blep OSC
bsebleposc_la_SOURCES = bsebleposc.cc bleposcdata.cc
bsebleposc_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
bsebleposc_la_LIBADD  = ../../bse/libbse-@MAJOR@.la $(LIBBSE_LIBS)
plugin_LTLIBRARIES = bsebleposc.la

noinst_PROGRAMS = designblep testblep

progs_ldadd    	  = ../../bse/libbse-@MAJOR@.la $(LIBBSE_LIBS)

designblep_SOURCES = designblep.cc
designblep_LDADD   = $(progs_ldadd)

testblep_SOURCES  = testblep.cc bleposcdata.cc
testblep_CXXFLAGS = $(AM_CXXFLAGS)
testblep_LDADD    = $(progs_ldadd)

EXTRA_DIST += bsebleposc.idl

# setup generation of C++ plugins from idl files
SFIDL = ../../sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)
%.genidl.hh: %.idl # $(SFIDL)
	$(SFIDL) --plugin  --macro $(<F) $(SFIDL_INC) -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )

$(srcdir)/bsebleposc.cc: bsebleposc.genidl.hh

# generate blepdata.hh
bleposcdata.cc: designblep
	./designblep > bleposcdata.cc || ( rm $@ ; exit 1 )

CLEANFILES += bsebleposc.genidl.hh bleposcdata.cc

