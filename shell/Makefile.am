# BSW-SCM - Bedevilled Sound Engine Scheme Wrapper
# Copyright (C) 2001-2002 Tim Janik
#
## Makefile.am for BSW-Shell

INCLUDES = -I$(top_srcdir) -I. $(BSW_SHELL_CFLAGS) -DG_DISABLE_CONST_RETURNS

#
# built sources
#
built_sources = # bswscm-genglue.c
MAINTAINERCLEANFILES += $(built_sources)
EXTRA_DIST += $(built_sources)
# temporary build files
gen_sources = xgen-bsggc
CLEANFILES += $(gen_sources)

#
# programs to build
#
bin_PROGRAMS = bswshell
# libraries to link programs against
programs_ldadd = $(top_builddir)/bse/libbse.la $(top_builddir)/bsw/libbsw.la
# source setups
bswshell_SOURCES = bswshell.c bswscminterp.c
bswshell_LDADD = $(programs_ldadd) $(BSW_SHELL_LIBS)
bswscminterp.c: # bswscm-genglue.c
EXTRA_DIST += bswscminterp.h


#
# scripts
#
scriptdir = $(bswscriptdir)
script_DATA = $(public_scripts)
public_scripts = $(strip	\
	bsw-scm-glue.boot	\
	progressor-example.scm	\
	test.scm		\
)
EXTRA_DIST += $(public_scripts)
