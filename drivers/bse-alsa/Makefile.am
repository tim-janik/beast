# BSE-ALSA - Bedevilled Sound Engine ALSA Driver Package
# Copyright (C) 2004 and Tim Janik
#
## Makefile.am for BSE-ALSA

# require automake 1.4
AUTOMAKE_OPTIONS = 1.4

INCLUDES = -I$(srcdir) -I$(builddir) $(BSE_CFLAGS)
# we specify the logging domain here
DEFS += $(strip -DG_LOG_DOMAIN=\"BSE-ALSA\")

# extra dependancies
configure: acbeast.m4

drivers_ldflags     = -avoid-version
drivers_libs        = $(BSE_LIBS)
driversdir          = $(bsedriverinstalldir)
drivers_LTLIBRARIES = bsepcmdevice-alsa.la



##
## Plugin bsepcmdevice-alsa
##
bsepcmdevice_alsa_la_SOURCES = bsepcmdevice-alsa.c
bsepcmdevice_alsa_la_LDFLAGS = -module $(drivers_ldflags)
bsepcmdevice_alsa_la_LIBADD  = $(drivers_libs) $(ALSA_LIBS)
EXTRA_HEADERS               += bsepcmdevice-alsa.h


EXTRA_DIST += $(strip		\
	acbeast.m4		\
)
