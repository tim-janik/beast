# Documentation utilities
# Copyright (C) 2002 Tim Janik
#
## Makefile.am for BEAST

SUBDIRS =

INCLUDES += -I${top_srcdir} -I. $(GLIB_CFLAGS)

#
# programs to build
#
noinst_PROGRAMS = xmlantispace
# source files
xmlantispace_SOURCES = xmlantispace.c
xmlantispace_LDADD = $(GLIB_LIBS)
EXTRA_DIST += xmlantispace.h
