# SHELL - Bedevilled Sound Engine Shell
# Copyright (C) 2001-2003 Tim Janik
#
## Makefile.am for bsesh

INCLUDES = -I$(top_srcdir) -I. $(BSE_SHELL_CFLAGS) -DG_DISABLE_CONST_RETURNS

DEFS += $(strip                       	\
	-DG_LOG_DOMAIN=\"bsesh\"       	\
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
bin_PROGRAMS = bsesh @BSESH_BINARY@		# this expands to bsesh-$BSE_VERSION
EXTRA_PROGRAMS = bsesh-\$(BSE_VERSION)		# make automake-1.4 eat bsesh____BSE_VERSION_ variables
bsesh____BSE_VERSION__SOURCES = bsesh.c bsescminterp.c cxxdummy.cc
bsesh____BSE_VERSION__LDADD = $(programs_ldadd) $(BSE_SHELL_LIBS)
bsesh_SOURCES = suidmain-tmp.c
SUIDMAIN_DEFS=-DSUIDMAIN_EXECUTABLE=\"$(bindir)/@BSESH_BINARY@\"
# libraries to link programs against
programs_ldadd = $(top_builddir)/bsw/libbsw.la $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la
# source setups
bsescminterp.c: # bsescm-genglue.c
EXTRA_DIST += bsescminterp.h

# always fetch newest suidmain.c from beast-gtk/ source
suidmain-tmp.c: $(top_srcdir)/beast-gtk/suidmain.c
	cp $< $@
CLEANFILES += suidmain-tmp.c

#
# make bsesh wrapper suid to allow renicing
#
install-exec-local: install-binPROGRAMS
	true \
	&& ( chown root $(bindir)/bsesh \
	&&   chmod 4755 $(bindir)/bsesh ) \
	|| ( echo "*** WARNING *** bsesh needs to be installed as root to allow renicing ***" && sleep 5 )


#
# scripts
#
shellscriptdir = $(bsescriptdir)
shellscript_DATA = bse-scm-glue.boot

EXTRA_DIST += $(shellscript_DATA)
