# BLib - BSE/BSI helper library
# Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BLib

SUBDIRS =

INCLUDES = -I$(top_srcdir) $(BLIB_CFLAGS)

#
# libraries to compile and install
#
lib_LTLIBRARIES = libblib.la

# due to automake-1.3 not featuring target specific defines,
# the G_LOG_DOMAIN needs to be special cased, depending on
# the source file sets ;(
# due to automake-1.3 not featuring target specific defines,
# the G_LOG_DOMAIN needs to be special cased, depending on
# the source file sets ;(
DEFS += @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"BLib\" \
@STRIP_END@

# libtool stuff: set version and export symbols for resolving
libblibincludedir = $(includedir)/blib
libblib_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic \
@STRIP_END@
libblib_la_LIBADD = $(BLIB_LIBS) -lm


#
# setup source file variables
#
# BLib header files for public installation (non-generated)
blib_public_h_sources = @STRIP_BEGIN@ \
	blib.h \
	bmain.h \
	btype.h \
	bparam.h \
	bhashmap.h \
	glib-gvalue.h \
	glib-gparam.h \
	glib-gparamspecs.h \
	glib-extra.h \
	glib-genums.h \
	glib-gobject.h \
	glib-gtype.h \
	glib-gvaluecollector.h \
	glib-includes.h \
@STRIP_END@
# BLib header files that don't get installed
blib_private_h_sources = @STRIP_BEGIN@ \
@STRIP_END@
# BLib C sources to build the library from
blib_c_sources = @STRIP_BEGIN@ \
	bmain.c \
	btype.c \
	bparam.c \
	bhashmap.c \
	glib-gvalue.c \
	glib-gparam.c \
	glib-gparamspecs.c \
	glib-extra.c \
	glib-genums.c \
	glib-gobject.c \
	glib-gtype.c \
@STRIP_END@
# we use our own built_sources variable rules instead of BUILT_SOURCES,
# since automake handles BUILT_SOURCES strange and wrong on some occasions
# (mostly wrt dependancies)
# we generate frequently rebuild files piggyback on a stamp file, so sources
# depending on them only get rebuild when the built source actually changed
# content
#
# built sources that don't get installed
blib_built_sources = @STRIP_BEGIN@ \
@STRIP_END@
# built sources that get installed with the header files
blib_built_public_sources = @STRIP_BEGIN@ \
@STRIP_END@
# non-header sources (headers should be specified in the above variables)
# that don't serve as direct make target sources, i.e. they don't have
# their own .lo rules and don't get publically installed
blib_extra_sources = @STRIP_BEGIN@ \
	blibconfig.h.in \
@STRIP_END@

#
# setup BLib sources and their dependancies
#
blib_h_sources = $(blib_private_h_sources) $(blib_public_h_sources) $(blib_built_public_sources)
libblibinclude_HEADERS = $(blib_public_h_sources) $(blib_built_public_sources)
libblib_la_SOURCES = $(blib_c_sources)
MAINTAINERCLEANFILES += $(blib_built_public_sources) $(blib_built_sources)
EXTRA_HEADERS +=
EXTRA_DIST += $(blib_private_h_sources)
EXTRA_DIST += $(blib_built_sources) $(blib_built_public_sources) $(blib_extra_sources)

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources =
CLEANFILES += $(gen_sources)
COPYING.LIB: $(blib_built_public_sources) $(blib_built_sources)
$(OBJECTS): COPYING.LIB # this is our oldest-source-stamp

#
# convenience targets for the builtsources
#
.PHONY: builtsources clean-builtsources
clean-builtsources:
	rm -f $(blib_built_public_sources) $(blib_built_sources)
builtsources:		$(blib_built_public_sources) $(blib_built_sources)


#
# private (birnet) dependancy to make TAGS file after each build
#
all: @MISSING_ETAGS@ @BIRNET_Y@ # TAGS


#
# other programs, we want to compile
#
noinst_PROGRAMS = testtypes
# source files
testtypes_SOURCES = testtypes.c
# link programs against BSE
progs_LDADD = libblib.la
testtypes_LDADD = $(progs_LDADD)


#
# auxillary files
#
EXTRA_DIST += \
	TODO
