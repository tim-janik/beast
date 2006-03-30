# BSE - Bedevilled Sound Engine tests
# Copyright (C) 2003 Tim Janik
# Copyright (C) 2005 Stefan Westerfeld
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)
DEFS     += -DG_LOG_DOMAIN="\"BSE-TEST\"" # -DG_DISABLE_CONST_RETURNS

TESTS = loophandle
noinst_PROGRAMS = $(TESTS)
progs_ldadd = $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la
loophandle_SOURCES = loophandle.c
loophandle_LDADD = $(progs_ldadd)
