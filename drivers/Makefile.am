# BSE-Plugins - Better Sound Engine dynamic Plugins
include $(top_srcdir)/Makefile.decl

INCLUDES    += -I$(top_srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ $(strip -DG_LOG_DOMAIN=\"BSE-ALSA\")
AM_CXXFLAGS  = $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE

# Plugin bsedevice-alsa
driversdir          = $(bsedriverdir)
drivers_LTLIBRARIES = bsepcmdevice-alsa.la bsemididevice-alsa.la
drivers_ldflags     = -avoid-version
drivers_libs        = $(top_builddir)/bse/libbse.la $(BSE_LIBS) $(SFI_LIBS) $(RAPICORN_LIBS)
EXTRA_DIST=

# Plugin bsepcmdevice-alsa
bsepcmdevice_alsa_la_SOURCES = bsepcmdevice-alsa.cc
bsepcmdevice_alsa_la_LDFLAGS = -module $(drivers_ldflags)
bsepcmdevice_alsa_la_LIBADD  = $(drivers_libs) $(ALSA_LIBS)
EXTRA_DIST                  += bsepcmdevice-alsa.hh

# Plugin bsemididevice-alsa
bsemididevice_alsa_la_SOURCES = bsemididevice-alsa.cc
bsemididevice_alsa_la_LDFLAGS = -module $(drivers_ldflags)
bsemididevice_alsa_la_LIBADD  = $(drivers_libs) $(ALSA_LIBS)
EXTRA_DIST                   += bsemididevice-alsa.hh
