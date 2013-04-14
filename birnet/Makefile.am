# Birnet
include $(top_srcdir)/Makefile.decl

SUBDIRS		= . tests
INCLUDES       += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS           += @DEFINE__FILE_DIR__@ -DBIRNET_LOG_DOMAIN=\"BIRNET\" -D_BIRNET_SOURCE_EXTENSIONS
AM_CXXFLAGS	= $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE $(BIRNET_CFLAGS)
GLIB_MKENUMS    = glib-mkenums

birnet_headers = $(strip 		\
	birnetcdefs.h			\
	birnetutils.hh			\
)
EXTRA_DIST += configure.inc acbirnet.m4

noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd     = $(BIRNET_LIBS) $(RAPICORN_LIBS) -lm
