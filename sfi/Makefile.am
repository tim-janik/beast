# SFI - Synthesis Fusion Kit Interface
# Copyright (C) 2002 Tim Janik
#
## Makefile.am for SFI

SUBDIRS =

INCLUDES += -I$(top_srcdir) -I$(top_builddir) $(SFI_CFLAGS)

# supported defines: SFI_DISABLE_DEBUG

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

sfi_public_headers = $(strip \
	sfistore.h	sficomwire.h	\
	glib-extra.h	sfi.h		\
	sfivmarshal.h	sfiglue.h	sfigluecodec.h	sfiglueproxy.h	\
	sfinote.h	sfiparams.h	sfiprimitives.h	sfiserial.h	\
	sfitime.h	sfitypes.h	sfivalues.h	sfiustore.h	\
	sfilog.h	sfithreads.h	sfimemory.h	sficomport.h	\
)
sfi_c_sources = $(strip \
	sfistore.c	sficomwire.c	\
	glib-extra.c	\
	sfivmarshal.c	sfiglue.c	sfigluecodec.c	sfiglueproxy.c	\
	sfinote.c	sfiparams.c	sfiprimitives.c	sfiserial.c	\
	sfitime.c	sfitypes.c	sfivalues.c	sfiustore.c	\
	sfilog.c	sfithreads.c	sfimemory.c	sficomport.c	\
	$(TOYPROF_C_SOURCES)	\
)
sfi_extra_sources = $(strip \
	gbsearcharray.h \
)
sfi_built_sources = $(strip \
)

$(sfi_c_sources): $(sfi_built_sources)

libsfiinclude_HEADERS = $(sfi_public_headers) sficonfig.h
libsfi_la_SOURCES = $(sfi_c_sources)

#
# generated sources
#
SFIDL = $(top_builddir)/sfi/sfidl
temp_files =  xgen-tih xgen-tic xgen-sch
CLEANFILES += $(temp_files)
CLEANFILES += testidl.h testidl.c
testidl.h: $(srcdir)/testidl.sfidl sfidl
	$(SFIDL) --header --implementation --boxed $(srcdir)/testidl.sfidl > xgen-tih \
	&& mv xgen-tih testidl.h
testidl.c: $(srcdir)/testidl.sfidl sfidl
	$(SFIDL) --source --implementation --boxed --init test_types_init $(srcdir)/testidl.sfidl > xgen-tic \
	&& mv xgen-tic testidl.c
EXTRA_DIST += testidl.sfidl

sficonfig.h: $(top_builddir)/config.status Makefile
	echo "/* Generated data from $< (by make $@) */" > xgen-sch \
	&& echo >> xgen-sch \
	&& echo "#define SFI_SIZEOF_PTH_MUTEX_T         (@SFI_SIZEOF_PTH_MUTEX_T@)" >> xgen-sch \
	&& echo "#define SFI_SIZEOF_PTH_COND_T          (@SFI_SIZEOF_PTH_COND_T@)" >> xgen-sch \
	&& echo "#define SFI_HAVE_MUTEXATTR_SETTYPE     (@SFI_HAVE_MUTEXATTR_SETTYPE@ && \\" >> xgen-sch \
	&& echo "                                        SFI_SIZEOF_PTH_MUTEX_T && \\" >> xgen-sch \
	&& echo "                                        SFI_SIZEOF_PTH_COND_T)" >> xgen-sch \
	&& echo >> xgen-sch \
	&& echo "/* Generated data ends here */" >> xgen-sch \
	&& (cmp -s xgen-sch $@ || cp xgen-sch $@) \
	&& rm -f xgen-sch
CLEANFILES += sficonfig.h xgen-sch
$(srcdir)/sfithreads.c: sficonfig.h
$(OBJECTS): sficonfig.h

#
# programs to build
#
noinst_PROGRAMS = sfidl testsfi
# source files
sfidl_SOURCES = sfidl.cc sfidl-namespace.cc sfidl-options.cc sfidl-parser.cc glib-extra.c
EXTRA_DIST += sfidl-namespace.h sfidl-options.h sfidl-parser.h
progs_extra_sources += sfidl-namespace.h
sfidl_LDADD = $(SFI_LIBS) -lm
testsfi_SOURCES = testsfi.c
testsfi_LDADD = libsfi.la
${srcdir}/testsfi.c: testidl.h testidl.c

#
# TESTS
#
TESTS = testsfi

#
# TOYPROF: ppor man's profiling toy
#
TOYPROF_H_SOURCES = toyprof.h toyprof-mem.h
TOYPROF_C_SOURCES = toyprof.c toyprof-mem.c
EXTRA_DIST += toyprof.pl toyprof.README $(TOYPROF_H_SOURCES)


EXTRA_DIST += $(sfi_public_headers) $(sfi_extra_sources) $(progs_extra_sources) $(sfi_built_sources)
MAINTAINERCLEANFILES += $(sfi_built_sources)
