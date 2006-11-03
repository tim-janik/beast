# BSE - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 2003 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS =

INCLUDES += -I$(top_srcdir) -I$(top_builddir) $(SFI_CFLAGS)

DEFS += -DG_LOG_DOMAIN=\"BSE-TEST\" # -DG_DISABLE_CONST_RETURNS

EXTRA_DIST += arrows.gp filter-defs.gp

noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd     = $(top_builddir)/bse/libbse.la

TESTS             += filtertest
SLOWTESTS         += filtertest
filtertest_SOURCES = filtertest.cc
filtertest_LDADD   = $(progs_ldadd)

TESTS          += testfft
testfft_SOURCES = testfft.c cxxdummy.cc
testfft_LDADD   = $(progs_ldadd)

TESTS          += testcxx
testcxx_SOURCES = testcxx.cc
testcxx_LDADD   = $(progs_ldadd)

TESTS              += subnormals 
PERFTESTS          += subnormals 
subnormals_SOURCES  = subnormals.cc subnormals-aux.cc
subnormals_LDADD    = $(progs_ldadd)
subnormals_CXXFLAGS = $(AM_CXXFLAGS) -ffast-math

TESTS             += loophandle
loophandle_SOURCES = loophandle.c
loophandle_LDADD   = $(progs_ldadd)

TESTS             += blocktests
PERFTESTS         += blocktests
blocktests_SOURCES = blocktests.cc
blocktests_LDADD   = $(progs_ldadd)

TESTS                 += resamplehandle
SLOWTESTS             += resamplehandle
PERFTESTS             += resamplehandle
resamplehandle_SOURCES = resamplehandle.cc
resamplehandle_LDADD   = $(progs_ldadd)
