# BSE - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 2003 Tim Janik
#
## Makefile.am for BSE tests

SUBDIRS =

INCLUDES += -I$(top_srcdir) -I$(top_builddir) $(SFI_CFLAGS)

DEFS += -DG_LOG_DOMAIN=\"BSE-TEST\" # -DG_DISABLE_CONST_RETURNS


TESTS = testwavechunk testfft
progs_ldadd = $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la
testwavechunk_SOURCES = testwavechunk.c cxxdummy.cc
testwavechunk_LDADD = $(progs_ldadd)
testfft_SOURCES = testfft.c cxxdummy.cc
testfft_LDADD = $(progs_ldadd)

noinst_PROGRAMS = cutvorbis magictest mathtests $(TESTS)
cutvorbis_SOURCES = cutvorbis.c cxxdummy.cc
cutvorbis_LDADD = $(progs_ldadd)
magictest_SOURCES = magictest.c cxxdummy.cc
magictest_LDADD = $(progs_ldadd)
mathtests_SOURCES = mathtests.c cxxdummy.cc
mathtests_LDADD = $(progs_ldadd)


EXTRA_DIST += $(strip \
)
