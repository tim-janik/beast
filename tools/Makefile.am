# BEAST - Bedevilled Audio System
# Copyright (C) 1996-2004 Tim Janik
#
## Makefile.am for beast/tools

INCLUDES += -I$(top_srcdir) -I$(top_builddir) $(BSE_CFLAGS)
DEFS += -DG_LOG_DOMAIN=\"BeastTools\" -DG_DISABLE_CONST_RETURNS #   """""""

#
# programs to build
#
noinst_PROGRAMS = bsewavetool bsefextract bsefcompare
bsewavetool_SOURCES = bsewavetool.c sfiutils.c bseloopfuncs.c
bsewavetool_LDADD = $(programs_ldadd)

bsefextract_SOURCES = bsefextract.cc
bsefextract_LDADD = $(programs_ldadd)

bsefcompare_SOURCES = bsefcompare.cc
bsefcompare_LDADD = $(programs_ldadd)

# libraries to link programs against
programs_ldadd = $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la

headers = $(strip	\
	bsewavetool.h	\
	sfiutils.h	\
	bseloopfuncs.h	\
)

#
# tests
#
TESTS = 

EXTRA_DIST += $(headers)
MAINTAINERCLEANFILES +=
