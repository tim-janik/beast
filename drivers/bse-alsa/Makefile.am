# BSE-ALSA - Bedevilled Sound Engine ALSA Driver Package
# Copyright (C) 2004 and Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.

INCLUDES = -I$(srcdir) -I$(top_builddir) $(BSE_CFLAGS)
# we specify the logging domain here
DEFS = $(strip -DG_LOG_DOMAIN=\"BSE-ALSA\")

# extra dependancies
configure: acbeast.m4

drivers_ldflags     = -avoid-version
drivers_libs        = $(BSE_LIBS)
driversdir          = $(bsedriverinstalldir)
drivers_LTLIBRARIES = bsepcmdevice-alsa.la bsemididevice-alsa.la
EXTRA_DIST=



##
## Plugin bsepcmdevice-alsa
##
bsepcmdevice_alsa_la_SOURCES = bsepcmdevice-alsa.c
bsepcmdevice_alsa_la_LDFLAGS = -module $(drivers_ldflags)
bsepcmdevice_alsa_la_LIBADD  = $(drivers_libs) $(ALSA_LIBS)
EXTRA_DIST                  += bsepcmdevice-alsa.h

##
## Plugin bsemididevice-alsa
##
bsemididevice_alsa_la_SOURCES = bsemididevice-alsa.c
bsemididevice_alsa_la_LDFLAGS = -module $(drivers_ldflags)
bsemididevice_alsa_la_LIBADD  = $(drivers_libs) $(ALSA_LIBS)
EXTRA_DIST                   += bsemididevice-alsa.h

EXTRA_DIST += acbeast.m4
