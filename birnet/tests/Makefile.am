# Birnet
# Copyright (C) 2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES       += -I$(top_srcdir) -I$(top_builddir) -I. $(BIRNET_CFLAGS)
DEFS	       += -DBIRNET_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID -DG_DISABLE_CONST_RETURNS

noinst_PROGRAMS  = $(TESTS)
progs_ldadd 	 = $(top_builddir)/birnet/libbirnet.o $(BIRNET_LIBS) -lm

TESTS		+= infotest
infotest_SOURCES = infotest.cc
infotest_LDADD	 = $(progs_ldadd)
TESTS		+= strings
SLOWTESTS       += strings
strings_SOURCES  = strings.cc
strings_LDADD	 = $(progs_ldadd)
TESTS		+= threads
PERFTESTS       += threads
threads_SOURCES	 = threads.cc
threads_LDADD	 = $(progs_ldadd)
TESTS		+= signal
signal_SOURCES	 = signal.cc
signal_LDADD	 = $(progs_ldadd)
TESTS		+= sorting
SLOWTESTS       += sorting
sorting_SOURCES  = sorting.cc
sorting_LDADD	 = $(progs_ldadd)
TESTS		+= datalist
datalist_SOURCES = datalist.cc
datalist_LDADD	 = $(progs_ldadd)
