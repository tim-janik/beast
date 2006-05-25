# BSE-PortAudio - Bedevilled Sound Engine PortAudio Driver Package
# Copyright (C) 2004 Tim Janik
# Copyright (C) 2004 Stefan Westerfeld
#
## GNU Lesser General Public License version 2 or any later version.

INCLUDES = -I$(srcdir) -I$(top_builddir) $(BSE_CFLAGS)
# we specify the logging domain here
DEFS = $(strip -DG_LOG_DOMAIN=\"BSE-PortAudio\")
# make automake1.9 happy
EXTRA_DIST =

# extra dependancies
configure: acbeast.m4

drivers_ldflags     = -avoid-version
drivers_libs        = $(BSE_LIBS)
driversdir          = $(bsedriverinstalldir)
drivers_LTLIBRARIES = bsepcmdevice-portaudio.la



##
## Plugin bsepcmdevice-portaudio
##
bsepcmdevice_portaudio_la_SOURCES = bsepcmdevice-portaudio.cc
bsepcmdevice_portaudio_la_LDFLAGS = -module $(drivers_ldflags)
bsepcmdevice_portaudio_la_LIBADD  = $(drivers_libs) $(PORTAUDIO_LIBS)
EXTRA_DIST                       += bsepcmdevice-portaudio.hh

EXTRA_DIST += $(strip		\
	${EXTRA_HEADERS}	\
	acbeast.m4		\
)
