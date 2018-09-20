# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I.
DEFS        += $(strip -DG_LOG_DOMAIN=\"BSE-ALSA\") -DBSE_PLUGIN_COMPILATION
AM_CXXFLAGS += $(BSE_CFLAGS) $(RAPICORN_CFLAGS)

# Plugin bsedevice-alsa
driverdir          = $(pkginstalldir)/drivers
driver_LTLIBRARIES = bsepcmdevice-alsa.la bsemididevice-alsa.la
driver_ldflags     = -avoid-version
driver_libs        = ../bse/libbse-@MAJOR@.la $(SFI_LIBS)

# Plugin bsepcmdevice-alsa
bsepcmdevice_alsa_la_SOURCES = bsepcmdevice-alsa.cc
bsepcmdevice_alsa_la_LDFLAGS = $(AM_LDFLAGS) -module $(driver_ldflags)
bsepcmdevice_alsa_la_LIBADD  = $(driver_libs) $(ALSA_LIBS)

# Plugin bsemididevice-alsa
bsemididevice_alsa_la_SOURCES = bsemididevice-alsa.cc
bsemididevice_alsa_la_LDFLAGS = $(AM_LDFLAGS) -module $(driver_ldflags)
bsemididevice_alsa_la_LIBADD  = $(driver_libs) $(ALSA_LIBS)
