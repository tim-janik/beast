# SFI - Synthesis Fusion Kit Interface
# Copyright (C) 2002 Tim Janik
#
## Makefile.am for SFI

SUBDIRS =

INCLUDES += -I.. $(SFI_CFLAGS) -I.

# public libraries
lib_LTLIBRARIES = libsfi.la
DEFS += -DG_LOG_DOMAIN=\"SFI\" -DG_DISABLE_CONST_RETURNS
libsfiincludedir = $(includedir)/sfi
libsfi_la_LDFLAGS = $(strip \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic \
)
libsfi_la_LIBADD = $(SFI_LIBS) -lm

sfi_headers = $(strip \
	glib-extra.h	\
	sfi.h		\
	sfiglue.h	\
	sfigluecodec.h	\
	sfinote.h	\
	sfiparams.h	\
	sfiprimitives.h	\
	sfiserial.h	\
	sfitime.h	\
	sfitypes.h	\
	sfivalues.h	\
	sfivcall.h	\
)
sfi_c_sources = $(strip \
	glib-extra.c	\
	sfiglue.c	\
	sfigluecodec.c	\
	sfinote.c	\
	sfiparams.c	\
	sfiprimitives.c	\
	sfiserial.c	\
	sfitime.c	\
	sfitypes.c	\
	sfivalues.c	\
	sfivcall.c	\
)
sfi_extra_sources = $(strip \
	sfigluesignal.c	\
)
sfi_built_sources = $(strip \
)

GLIB_MKENUMS = glib-mkenums
temp_files =  xgen-smh xgen-smc
stamp_files = # gxkgentypes.stamp

$(sfi_c_sources): $(sfi_built_sources)

libsfi_la_SOURCES = $(sfi_c_sources)


#
# program to build
#
noinst_PROGRAMS = sfidl testsfi
# source files
sfidl_SOURCES = sfidl.cc sfidl-namespace.cc
progs_extra_sources += sfidl-namespace.h
testsfi_SOURCES = testsfi.c
# program linkage
progs_LDADD = libsfi.la
sfidl_LDADD = $(progs_LDADD)
testsfi_LDADD = $(progs_LDADD)

#
# TESTS
#
TESTS = testsfi


EXTRA_DIST += $(sfi_headers) $(sfi_extra_sources) $(sfi_built_sources)
MAINTAINERCLEANFILES += $(sfi_built_sources) $(stamp_files)
CLEANFILES += $(temp_files)
