# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BSE Plugins

SUBDIRS = icons

INCLUDES += -I$(top_srcdir) $(BSE_CFLAGS) -DG_LOG_DOMAIN=BSE_PLUGIN_NAME
INCLUDES += -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\""

# install plugins under:
plugindir = $(bseplugindir)

# all plugins want to link against:
plugin_libs = $(BSE_LIBS) -lm

# create plugin list
Makefile.plugins: $(srcdir)/listplugins.sh *.c Makefile.am
	$(srcdir)/listplugins.sh *.c >$@

EXTRA_DIST += $(strip \
        $(EXTRA_HEADERS) \
	listplugins.sh \
	TODO \
)

## include generated plugin Makefile
include Makefile.plugins
## Makefile.plugins contains a generated make rule list of the *.c
## files in this directory, e.g. it'd list a given foo.c as:
##
##foo_la_SOURCES = foo.c
##foo_la_LDFLAGS = -module -avoid-version
##foo_la_LIBADD  = $(plugin_libs)
##EXTRA_HEADERS += foo.h
##
##plugin_LTLIBRARIES += foo.la
