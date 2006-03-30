# Birnet
# Copyright (C) 2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES	= -I$(top_srcdir) -I$(top_builddir) -I. $(BIRNET_CFLAGS)
DEFS		= -DG_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID -DG_DISABLE_CONST_RETURNS

TESTS 		= ring threads signal
noinst_PROGRAMS = $(TESTS)
progs_ldadd 	= $(top_builddir)/birnet/libbirnet.o $(BIRNET_LIBS) -lm
ring_SOURCES	= ring.cc testutils.h
ring_LDADD	= $(progs_ldadd)
threads_SOURCES	= threads.cc testutils.h
threads_LDADD	= $(progs_ldadd)
signal_SOURCES	= signal.cc
signal_LDADD	= $(progs_ldadd)

EXTRA_DIST += $(strip	\
	testutils.h	\
)
