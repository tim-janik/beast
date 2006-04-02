# SHELL - Bedevilled Sound Engine Shell
# Copyright (C) 2001-2003 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I. -I$(top_builddir) $(BSE_SHELL_CFLAGS) -DG_DISABLE_CONST_RETURNS

DEFS += $(strip                       	\
	-DG_LOG_DOMAIN=\"BSESH\"       	\
	$(SUIDMAIN_DEFS)                \
)

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
bin_PROGRAMS = bsesh-$(BIN_VERSION)		# this expands to bsesh-0.7.0 or similar
bsesh___BIN_VERSION__SOURCES = bsesh.c bsescminterp.c cxxdummy.cc
bsesh___BIN_VERSION__LDADD = $(programs_ldadd) $(BSE_SHELL_LIBS)

# libraries to link programs against
programs_ldadd = $(top_builddir)/bse/libbse.la
# source setups
bsescminterp.c: # bsescm-genglue.c
EXTRA_DIST += bsescminterp.h


#
# scripts
#
shellscriptdir = $(bsescriptdir)
shellscript_DATA = bse-scm-glue.boot

EXTRA_DIST += $(shellscript_DATA)
