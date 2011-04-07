# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 2004-2011 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I. $(BSE_CFLAGS)
DEFS = $(strip -DG_LOG_DOMAIN=\"BSE-ALSA\")

# Plugin bsedevice-alsa
driversdir          = $(bsedriverdir)
drivers_LTLIBRARIES = bsepcmdevice-alsa.la bsemididevice-alsa.la
drivers_ldflags     = -avoid-version
drivers_libs        = $(top_builddir)/bse/libbse.la $(BSE_LIBS)
EXTRA_DIST=

# Plugin bsepcmdevice-alsa
bsepcmdevice_alsa_la_SOURCES = bsepcmdevice-alsa.c
bsepcmdevice_alsa_la_LDFLAGS = -module $(drivers_ldflags)
bsepcmdevice_alsa_la_LIBADD  = $(drivers_libs) $(ALSA_LIBS)
EXTRA_DIST                  += bsepcmdevice-alsa.h

# Plugin bsemididevice-alsa
bsemididevice_alsa_la_SOURCES = bsemididevice-alsa.c
bsemididevice_alsa_la_LDFLAGS = -module $(drivers_ldflags)
bsemididevice_alsa_la_LIBADD  = $(drivers_libs) $(ALSA_LIBS)
EXTRA_DIST                   += bsemididevice-alsa.h
