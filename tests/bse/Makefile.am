# BSE - Bedevilled Sound Engine tests
# Copyright (C) 2003 Tim Janik
# Copyright (C) 2005 Stefan Westerfeld
#
## GNU Lesser General Public License version 2 or any later version.

TESTS = loophandle

CFLAGS   += $(EXTRA_CFLAGS)
CPPFLAGS += $(EXTRA_CXXFLAGS)
INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)

DEFS += -DG_LOG_DOMAIN="\"BSE-TEST\"" # -DG_DISABLE_CONST_RETURNS

progs_ldadd = $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la
loophandle_SOURCES = loophandle.c
loophandle_LDADD = $(progs_ldadd)

noinst_PROGRAMS = $(TESTS)
