# BSE - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 2003 Tim Janik
#
## Makefile.am for BSE tests

SUBDIRS =

INCLUDES += -I$(top_srcdir) -I$(top_builddir) $(SFI_CFLAGS)

DEFS += -DG_LOG_DOMAIN=\"BSE-TEST\" # -DG_DISABLE_CONST_RETURNS

noinst_PROGRAMS = cutvorbis
cutvorbis_SOURCES = cutvorbis.c
cutvorbis_LDADD = $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la

TESTS = 

EXTRA_DIST += $(strip \
)
