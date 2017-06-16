# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I. -I$(top_builddir)
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN=\"BSESCM\" $(SUIDMAIN_DEFS)
AM_CXXFLAGS += $(BSESCM_CFLAGS) $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE -DG_DISABLE_CONST_RETURNS

#
# built sources
#
built_sources = # bsescm-genglue.c
MAINTAINERCLEANFILES += $(built_sources)
EXTRA_DIST += $(built_sources)
# temporary build files
gen_sources = xgen-bsggc
CLEANFILES += $(gen_sources)

# libraries to link programs against
programs_ldadd = ../bse/libbse-@MAJOR@.la $(LIBBSE_LIBS)

#
# programs to build
#
beastbin_PROGRAMS = bsescm
bsescm_SOURCES    = bsescm.cc bsescminterp.cc
bsescm_LDADD      = $(programs_ldadd) $(BSESCM_LIBS)

# source setups
bsescminterp.cc: # bsescm-genglue.c
EXTRA_DIST += bsescminterp.hh


#
# scripts
#
shellscriptdir = $(beastdatadir)/scripts
shellscript_DATA = bse-scm-glue.boot

EXTRA_DIST += $(shellscript_DATA)


#
# check
#
# many tests require a working bsescm. but the shell will only
# operate correctly after proper installation. so this rule
# ensures proper initialization upon make check.
installcheck-local: check-binary
.PHONY: check-binary
check-binary:
	@for p in $(beastbin_PROGRAMS) ; do				\
	  pp="$(DESTDIR)$(beastbindir)/$$p" ;				\
	  echo "TEST: test -x \"$$pp\"" ;				\
	  test -x "$$pp" ||						\
	    { echo "Failed to verify installation of executable: $$pp";	\
	      exit 1 ; }						\
	done
