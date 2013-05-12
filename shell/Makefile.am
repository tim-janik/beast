# SHELL - Better Sound Engine Shell
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I. -I$(top_builddir)
DEFS += @DEFINE__FILE_DIR__@ $(strip                       	\
	-DG_LOG_DOMAIN=\"BSESCM\"      	\
	$(SUIDMAIN_DEFS)                \
)
AM_CXXFLAGS  = $(BSESCM_CFLAGS) $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE -DG_DISABLE_CONST_RETURNS

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
programs_ldadd = $(top_builddir)/bse/libbse.la $(RAPICORN_LIBS)

#
# programs to build
#
bin_PROGRAMS = bsescm-$(BIN_VERSION)		# this expands to bsescm-0.7.0 or similar
bsescm___BIN_VERSION__SOURCES = bsescm.cc bsescminterp.cc
bsescm___BIN_VERSION__LDADD = $(programs_ldadd) $(BSESCM_LIBS)

# source setups
bsescminterp.cc: # bsescm-genglue.c
EXTRA_DIST += bsescminterp.hh


#
# scripts
#
shellscriptdir = $(bsescriptdir)
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
	@for p in $(bin_PROGRAMS) ; do					\
	  pp="$(DESTDIR)$(bindir)/$$p" ;				\
	  echo "TEST: test -x \"$$pp\"" ;				\
	  test -x "$$pp" ||						\
	    { echo "Failed to verify installation of executable: $$pp";	\
	      exit 1 ; }						\
	done
