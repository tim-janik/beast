# BSW - Bedevilled Sound Engine Wrapper
# Copyright (C) 2000-2001 Tim Janik
#
## Makefile.am for BSW

# SUBDIRS = # icons

INCLUDES = -I$(top_srcdir) $(BSE_CFLAGS) -DG_DISABLE_CONST_RETURNS

#
# libraries to compile and install
#
lib_LTLIBRARIES = libbsw.la

# due to automake-1.3 not featuring target specific defines,
# the G_LOG_DOMAIN needs to be special cased, depending on
# the source file sets ;(
DEFS += @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=bsw_log_domain_bsw \
@STRIP_END@

# libtool stuff: set version and export symbols for resolving
libbswincludedir = $(includedir)/bsw
libbsw_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic \
@STRIP_END@
libbsw_la_LIBADD = # $(top_builddir)/bse/libbse.la # -lm


#
# setup source file variables
#
# BSW header files for public installation (non-generated)
bsw_public_h_sources = @STRIP_BEGIN@ \
	bsw.h \
	bswproxy.h \
@STRIP_END@
# BSW header files that don't get installed
bsw_private_h_sources = @STRIP_BEGIN@ \
@STRIP_END@
# BSW C sources to build the library from
bsw_c_sources = @STRIP_BEGIN@ \
	bswproxy.c \
	bswgenapi.c \
@STRIP_END@
# source to built, that don't get installed
bsw_built_sources = @STRIP_BEGIN@ \
	stamp-bswgenapi.h \
	bswgenapi.c \
@STRIP_END@
# built sources that get installed with the header files
bsw_built_public_sources = @STRIP_BEGIN@ \
	bswgenapi.h \
@STRIP_END@
# non-header sources (headers should be specified in the above variables)
# that don't serve as direct make target sources, i.e. they don't have
# their own .lo rules and don't get publically installed
bsw_extra_sources = @STRIP_BEGIN@ \
@STRIP_END@

#
# setup BSW sources and their dependancies
#
bsw_h_sources = $(bsw_private_h_sources) $(bsw_public_h_sources) $(bsw_built_public_sources)
libbswinclude_HEADERS = $(bsw_public_h_sources) $(bsw_built_public_sources)
libbsw_la_SOURCES = $(bsw_c_sources)
MAINTAINERCLEANFILES += $(bsw_built_public_sources) $(bsw_built_sources)
EXTRA_HEADERS +=
EXTRA_DIST += $(bsw_private_h_sources)
EXTRA_DIST += $(bsw_built_sources) $(bsw_built_public_sources) $(bsw_extra_sources)

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-sbpah xgen-bpac
CLEANFILES += $(gen_sources)
COPYING: $(bsw_built_public_sources) $(bsw_built_sources)
$(libbsw_la_OBJECTS): COPYING # this is our oldest-source-stamp
# initial creation of the real stamp-* files
bswgenapi.h: @MISSING_PERL5@		# never add deps here
	test -f "$(srcdir)/$@" || touch $(srcdir)/$@
# normal autogeneration rules
$(srcdir)/stamp-bswgenapi.h: @MISSING_PERL5@ mkapi
	$(top_builddir)/bsw/mkapi --header >xgen-sbpah \
	&& (cmp -s xgen-sbpah $(srcdir)/bswgenapi.h || cp xgen-sbpah $(srcdir)/bswgenapi.h) \
	&& rm -f xgen-sbpah \
	&& echo timestamp > $@
$(srcdir)/bswgenapi.c: @MISSING_PERL5@ bswgenapi.h mkapi
	$(top_builddir)/bsw/mkapi --body >xgen-sbpac \
	&& cp xgen-sbpac $(srcdir)/bswgenapi.c \
	&& rm -f xgen-sbpac


#
# convenience targets for the builtsources
#
.PHONY: builtsources clean-builtsources
clean-builtsources:
	cd $(srcdir) \
	&& rm -f $(bsw_built_public_sources) $(bsw_built_sources)
	rm -f $(bsw_built_public_sources) $(bsw_built_sources)
builtsources: $(bsw_built_public_sources) $(bsw_built_sources)


#
# private (birnet) dependancy to make TAGS file after each build
#
all: @MISSING_ETAGS@ @BIRNET_Y@ # TAGS


#
# other programs, we want to compile
#
noinst_PROGRAMS = mkapi testbsw
# source files
mkapi_SOURCES = mkapi.c
testbsw_SOURCES = testbsw.c
# link programs against BSW
progs_LDADD = $(top_builddir)/bse/libbse.la
mkapi_LDADD = $(progs_LDADD)
testbsw_LDADD = libbsw.la $(progs_LDADD)


#
# auxillary files
#
EXTRA_DIST += #	TODO
