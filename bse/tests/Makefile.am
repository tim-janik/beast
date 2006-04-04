# BSE - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 2003 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS =

INCLUDES += -I$(top_srcdir) -I$(top_builddir) $(SFI_CFLAGS)

DEFS += -DG_LOG_DOMAIN=\"BSE-TEST\" # -DG_DISABLE_CONST_RETURNS

TESTS = testfft testcxx subnormals loophandle
noinst_PROGRAMS = $(TESTS)
progs_ldadd = $(top_builddir)/bse/libbse.la
testfft_SOURCES = testfft.c cxxdummy.cc
testfft_LDADD = $(progs_ldadd)
testcxx_SOURCES = testcxx.cc
testcxx_LDADD = $(progs_ldadd)
subnormals_SOURCES = subnormals.cc subnormals-aux.cc
subnormals_LDADD = $(progs_ldadd)
subnormals_CXXFLAGS = # -ffast-math
loophandle_SOURCES = loophandle.c
loophandle_LDADD = $(progs_ldadd)
