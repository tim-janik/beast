# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BSE Plugins

INCLUDES = -I$(top_srcdir) $(BSE_CFLAGS)

# install plugins under:
plugindir = $(bselibdir)/plugins

# all plugins want to link against:
plugin_libs = $(BSE_LIBS) -lm

# create plugin list
Makefile.plugins: $(srcdir)/listplugins.sh *.c
	$(srcdir)/listplugins.sh *.c >$@

EXTRA_DIST += $(strip \
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
##
##plugin_LTLIBRARIES += foo.la
