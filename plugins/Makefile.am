# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998-2002 Tim Janik
#
## Makefile.am for BSE Plugins

SUBDIRS = icons . freeverb evaluator

INCLUDES += -I$(top_srcdir) -I. $(BSE_CFLAGS) -DG_LOG_DOMAIN=BSE_PLUGIN_NAME
INCLUDES += -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\""

# install plugins under:
plugindir = $(bseplugindir)

#
# C++ Plugins
#
plugin_idl_files = $(strip		\
	davchorus.idl			\
	bsebalance.idl			\
	artscompressor.idl		\
	davbassfilter.idl		\
	bsecontribsampleandhold.idl	\
	bsenoise.idl			\
	standardsaturator.idl		\
)
EXTRA_HEADERS += $(plugin_idl_files)
# setup generation of C++ plugins from idl files
SFIDL = ../sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)
%.gen-idl.h: %.idl $(SFIDL)
	$(SFIDL) --plugin  --macro $(<F) $(SFIDL_INC) -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )

plugins_built_sources =
Makefile.plugins: $(srcdir)/listplugins.sh $(PLUGIN_CFILES) Makefile.am
	$(srcdir)/listplugins.sh $(PLUGIN_CFILES) >$@
	$(srcdir)/listplugins.sh --idl $(plugin_idl_files:.idl=) >>$@

# old/unported plugin list
UNPORTED = $(strip		\
	bsefirfilter.[hc]	\
	davguitar.[hc]		\
)
PLUGIN_CFILES = $(strip		\
	bseadder.[hc]		\
	bseatandistort.[hc]	\
	bseiirfilter.[hc]	\
	bsemixer.[hc]		\
	bsemult.[hc]		\
	bsesimpleadsr.[hc]	\
	bsesequencer.[hc]	\
	davorgan.[hc]		\
	davxtalstrings.[hc]	\
	davsyndrum.[hc]		\
	davcanyondelay.[hc]	\
)
EXTRA_CFILES = $(strip		\
	bsesimpleadsr-aux.c	\
)

CLEANFILES += $(plugins_built_sources)

## include generated plugin Makefile
include Makefile.plugins
## Makefile.plugins contains generated plugin rules:
##
##foo_la_SOURCES         = foo.cc
##foo_la_LDFLAGS         = -module $(plugin_ldflags)
##foo_la_LIBADD          = $(plugin_libs)
##$(srcdir)/foo.cc:        foo.gen-idl.h
##plugins_built_sources += foo.gen-idl.h
##plugin_LTLIBRARIES    += foo.la
#
plugin_ldflags = -avoid-version
plugin_libs    = $(BSE_LIBS) -lm # $(top_builddir)/bse/libbse.la

EXTRA_DIST += $(strip \
        $(EXTRA_HEADERS) \
        $(EXTRA_CFILES) \
	listplugins.sh \
)

