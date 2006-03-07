# SFI - Synthesis Fusion Kit Interface
# Copyright (C) 2002 Tim Janik
#
## Makefile.am for SFI

INCLUDES += -I$(top_srcdir) -I$(top_builddir) $(SFI_CFLAGS)
DEFS     += -DG_LOG_DOMAIN=\"SFI\" -DG_DISABLE_CONST_RETURNS

sfi_public_headers = $(strip \
	sfistore.h	sficomwire.h	sfifilecrawler.h \
	glib-extra.h	\
	sfivmarshal.h	sfiglue.h	sfigluecodec.h	sfiglueproxy.h	\
	sfinote.h	sfiparams.h	sfiprimitives.h	sfiserial.h	\
	sfitime.h	sfitypes.h	sfivalues.h	sfiustore.h	\
	sfilog.h	sfithreads.h	sfimemory.h	sficomport.h	\
	sficxx.h	sfi.h		\
)
sfi_c_sources = $(strip \
	sfistore.c	sficomwire.c	sfifilecrawler.c \
	glib-extra.c	\
	sfivmarshal.c	sfiglue.c	sfigluecodec.c	sfiglueproxy.c	\
	sfinote.c	sfiparams.c	sfiprimitives.c	sfiserial.c	\
	sfitime.c	sfitypes.c	sfivalues.c	sfiustore.c	\
	sfilog.c	sfithreads.c	sfimemory.c	sficomport.c	\
	$(conditional_toyprof_sources)	\
)
sfi_extra_sources = $(strip \
	gbsearcharray.h \
)
sfi_built_sources = $(strip \
)

$(sfi_c_sources): $(sfi_built_sources)

# SFI library
noinst_LTLIBRARIES    = libsfi.la
libsfiincludedir      = $(includedir)/sfi
libsfiinclude_HEADERS = $(sfi_public_headers) sficonfig.h
libsfi_la_SOURCES     = $(sfi_c_sources)
libsfi_la_LDFLAGS     = -no-undefined # -Wl,-Bsymbolic
libsfi_la_LIBADD      = $(SFI_LIBS) -lm

#
# generated sources
#
SFIDL = $(top_builddir)/sfi/sfidl
CLEANFILES += testidl.h testidl.c
testidl.h: $(srcdir)/testidl.idl sfidl
	$(SFIDL) --host-c --header $(srcdir)/testidl.idl > xgen-$(@F) \
	&& mv xgen-$(@F) testidl.h
testidl.c: $(srcdir)/testidl.idl sfidl
	$(SFIDL) --host-c --source --init test_types_init $(srcdir)/testidl.idl > xgen-$(@F) \
	&& mv xgen-$(@F) testidl.c
EXTRA_DIST += testidl.idl

sficonfig.h: $(top_builddir)/config.status Makefile
	echo "/* Generated data from $< (by make $@) */" > xgen-$(@F) \
	&& echo >> xgen-$(@F) \
	&& echo "#define SFI_SIZEOF_PTH_MUTEX_T         (@SFI_SIZEOF_PTH_MUTEX_T@)" >> xgen-$(@F) \
	&& echo "#define SFI_SIZEOF_PTH_COND_T          (@SFI_SIZEOF_PTH_COND_T@)" >> xgen-$(@F) \
	&& echo "#define SFI_HAVE_MUTEXATTR_SETTYPE     (@SFI_HAVE_MUTEXATTR_SETTYPE@ && \\" >> xgen-$(@F) \
	&& echo "                                        SFI_SIZEOF_PTH_MUTEX_T && \\" >> xgen-$(@F) \
	&& echo "                                        SFI_SIZEOF_PTH_COND_T)" >> xgen-$(@F) \
	&& echo >> xgen-$(@F) \
	&& echo "/* Generated data ends here */" >> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@) \
	&& rm -f xgen-$(@F)
CLEANFILES += sficonfig.h
$(srcdir)/sfithreads.c: sficonfig.h
$(OBJECTS): sficonfig.h

#
# programs to build
#
# source files
common_idl_sources = sfidl-generator.cc sfidl-namespace.cc sfidl-options.cc sfidl-parser.cc \
  sfidl-factory.cc sfidl-typelist.cc sfidl-cbase.cc sfidl-clientc.cc sfidl-clientcxx.cc sfidl-corec.cc \
  sfidl-corecxx.cc sfidl-cxxbase.cc sfidl-hostc.cc glib-extra.c

noinst_PROGRAMS = testsfi testcxx testsfidl
sfidl_SOURCES = sfidl.cc $(common_idl_sources)
sfidl_LDADD = $(SFI_LIBS) -lm # libsfi.la

bin_PROGRAMS = sfidl

EXTRA_DIST += sfidl-generator.hh sfidl-namespace.hh sfidl-options.hh sfidl-parser.hh sfidl-factory.hh sfidl-typelist.hh
EXTRA_DIST += sfidl-cbase.hh sfidl-clientc.hh sfidl-clientcxx.hh sfidl-corec.hh sfidl-corecxx.hh sfidl-cxxbase.hh sfidl-hostc.hh

progs_extra_sources += sfidl-namespace.hh
testsfi_SOURCES = testsfi.c
testsfi_LDADD = libsfi.la $(SFI_LIBS) -lm
$(srcdir)/testsfi.c: testidl.h testidl.c
testcxx_SOURCES = testcxx.cc
testcxx_LDADD = libsfi.la $(SFI_LIBS) -lm
$(srcdir)/testcxx.cc: testidl.h testidl.c
testsfidl_SOURCES = testsfidl.cc $(common_idl_sources)
testsfidl_LDADD = libsfi.la $(SFI_LIBS) -lm
$(srcdir)/testsfidl.cc: testidl.h testidl.c

#
# TESTS
#
TESTS = testsfi testcxx testsfidl

#
# TOYPROF: poor man's profiling toy
#
TOYPROF_H_SOURCES = toyprof.h toyprof-mem.h
TOYPROF_C_SOURCES = toyprof.c toyprof-mem.c
EXTRA_DIST += toyprof.pl toyprof.README $(TOYPROF_H_SOURCES)
if IF_TOYPROF
conditional_toyprof_sources = $(TOYPROF_C_SOURCES)
endif


EXTRA_DIST += $(sfi_public_headers) $(sfi_extra_sources) $(progs_extra_sources) $(sfi_built_sources)
MAINTAINERCLEANFILES += $(sfi_built_sources)
