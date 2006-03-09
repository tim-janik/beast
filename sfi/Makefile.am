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
	sficxx.hh			sfimemory.h	sficomport.h	\
	sfi.h								\
)
sfi_all_sources = $(strip \
	sfistore.c	sficomwire.c	sfifilecrawler.c \
	glib-extra.c	\
	sfivmarshal.c	sfiglue.c	sfigluecodec.c	sfiglueproxy.c	\
	sfinote.c	sfiparams.c	sfiprimitives.c	sfiserial.c	\
	sfitime.c	sfitypes.c	sfivalues.c	sfiustore.c	\
	sficxx.cc			sfimemory.c	sficomport.c	\
	$(conditional_toyprof_sources)	\
)
sfi_extra_sources = $(strip \
	gbsearcharray.h \
)
sfi_built_sources = $(strip \
)

$(sfi_all_sources): $(sfi_built_sources)

# SFI library
# we want a partial shared library here. libtool creates that if the library
# name ends in .lo or .o. however automake doesn't know .lo or .o libraries,
# so we simply build an ordinary non-installed .la library and then use our
# own rule to build the .o from the .la pieces.
noinst_LTLIBRARIES    = libsfi.la
libsfiincludedir      = $(includedir)/sfi
libsfiinclude_HEADERS = $(sfi_public_headers)
libsfi_la_SOURCES     = $(sfi_all_sources)
libsfi_la_LDFLAGS     = -no-undefined # -Wl,-Bsymbolic
libsfi_la_LIBADD      = $(SFI_LIBS) -lm
# keep this .o rule in sync with the corresponding .la rule from Makefile.in
libsfi.o: $(libsfi_la_OBJECTS) $(libsfi_la_DEPENDENCIES)
	$(CXXLINK)  $(libsfi_la_LDFLAGS) $(libsfi_la_OBJECTS) # $(libsfi_la_LIBADD) $(LIBS)
all-am: libsfi.o
CLEANFILES += libsfi.o libsfi.lo

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

#
# programs to build
#
# source files
common_idl_sources = sfidl-generator.cc sfidl-namespace.cc sfidl-options.cc sfidl-parser.cc \
  sfidl-factory.cc sfidl-typelist.cc sfidl-cbase.cc sfidl-clientc.cc sfidl-clientcxx.cc sfidl-corec.cc \
  sfidl-corecxx.cc sfidl-cxxbase.cc sfidl-hostc.cc glib-extra.c

bin_PROGRAMS = sfidl
sfidl_SOURCES = sfidl.cc $(common_idl_sources)
sfidl_LDADD = $(SFI_LIBS) -lm # libsfi.la
EXTRA_DIST += sfidl-generator.hh sfidl-namespace.hh sfidl-options.hh sfidl-parser.hh sfidl-factory.hh sfidl-typelist.hh
EXTRA_DIST += sfidl-cbase.hh sfidl-clientc.hh sfidl-clientcxx.hh sfidl-corec.hh sfidl-corecxx.hh sfidl-cxxbase.hh sfidl-hostc.hh

noinst_PROGRAMS = testsfi testcxx testsfidl
progs_nosfi_LDADD = $(top_builddir)/birnet/libbirnet.o $(SFI_LIBS) -lm
progs_LDADD = $(top_builddir)/birnet/libbirnet.o libsfi.o $(SFI_LIBS) -lm
$(srcdir)/testsfi.c: testidl.h testidl.c
testsfi_SOURCES = testsfi.c dummy.cc
testsfi_LDADD = $(progs_LDADD)
testsfi_LDFLAGS =
$(srcdir)/testcxx.cc: testidl.h testidl.c
testcxx_SOURCES = testcxx.cc
testcxx_LDADD = $(progs_LDADD)
$(srcdir)/testsfidl.cc: testidl.h testidl.c
testsfidl_SOURCES = testsfidl.cc $(common_idl_sources)
testsfidl_LDADD = $(progs_nosfi_LDADD)

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


EXTRA_DIST += $(sfi_public_headers) $(sfi_extra_sources) $(sfi_built_sources)
MAINTAINERCLEANFILES += $(sfi_built_sources)
