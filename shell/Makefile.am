# SHELL - Bedevilled Sound Engine Shell
# Copyright (C) 2001-2003 Tim Janik
#
## Makefile.am for bsesh

INCLUDES = -I$(top_srcdir) -I. $(BSE_SHELL_CFLAGS) -DG_DISABLE_CONST_RETURNS

#
# built sources
#
built_sources = # bsescm-genglue.c
MAINTAINERCLEANFILES += $(built_sources)
EXTRA_DIST += $(built_sources)
# temporary build files
gen_sources = xgen-bsggc
CLEANFILES += $(gen_sources)

#
# programs to build
#
bin_PROGRAMS = bsesh
# libraries to link programs against
programs_ldadd = $(top_builddir)/bsw/libbsw.la $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la
# source setups
bsesh_SOURCES = bsesh.c bsescminterp.c cxxdummy.cc
bsesh_LDADD = $(programs_ldadd) $(BSE_SHELL_LIBS)
bsescminterp.c: # bsescm-genglue.c
EXTRA_DIST += bsescminterp.h


#
# scripts
#
scriptdir = $(bsescriptdir)
script_DATA = $(public_scripts)
public_scripts = $(strip	\
	bse-scm-glue.boot	\
	progressor-example.scm	\
	modules2grid.scm	\
)
EXTRA_DIST += $(public_scripts) test.scm
