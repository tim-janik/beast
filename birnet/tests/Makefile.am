# Birnet
# Copyright (C) 2006 Tim Janik
#
# GNU General Public License version 2 or (at your option) any later version.

INCLUDES	= -I$(top_srcdir) -I$(top_builddir) -I. $(BIRNET_CFLAGS)
DEFS		= -DG_LOG_DOMAIN='"birnet/tests/"__FILE__":PRG"' -DPARANOID -DG_DISABLE_CONST_RETURNS

noinst_PROGRAMS = $(TESTS)
TESTS 		= threads ring
progs_ldadd 	= $(top_builddir)/birnet/libbirnet.la
threads_SOURCES	= threads.cc testutils.h
threads_LDADD	= $(progs_ldadd)
ring_SOURCES	= ring.cc testutils.h
ring_LDADD	= $(progs_ldadd)

EXTRA_DIST += $(strip	\
	testutils.h	\
)
