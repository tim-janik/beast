# SFI - Synthesis Fusion Kit Interface
include $(top_srcdir)/Makefile.decl

SUBDIRS   = . tests
INCLUDES    += -I$(top_srcdir) -I$(top_builddir) $(SFI_CFLAGS)
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN=\"SFI\" -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS  = $(SFI_CFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE

sfi_public_headers = $(strip \
	sfistore.hh	sficomwire.hh	sfifilecrawler.hh \
	glib-extra.hh			sfiwrapper.hh	 \
	sfivmarshal.hh	sfiglue.hh	sfigluecodec.hh	sfiglueproxy.hh	\
	sfinote.hh	sfiparams.hh	sfiprimitives.hh	sfiserial.hh	\
	sfitime.hh	sfitypes.hh	sfivalues.hh	sfiustore.hh	\
	sficxx.hh	sfiring.hh	sfimemory.hh	sficomport.hh	\
	sfitests.hh	sfi.hh						\
)
sfi_all_sources = $(strip \
	sfistore.cc	sficomwire.cc	sfifilecrawler.cc \
	glib-extra.cc			sfiwrapper.cc	sfiprimitives.cc \
	sfivmarshal.cc	sfiglue.cc	sfigluecodec.cc	sfiglueproxy.cc	\
	sfinote.cc	sfiparams.cc			sfiserial.cc	\
	sfitime.cc	sfitypes.cc	sfivalues.cc	sfiustore.cc	\
	sficxx.cc	sfiring.cc	sfimemory.cc	sficomport.cc	\
	$(conditional_toyprof_sources)	\
)
sfi_extra_sources = $(strip \
	gbsearcharray.hh \
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
# programs to build
#
# source files
common_idl_sources = sfidl-generator.cc sfidl-namespace.cc sfidl-options.cc sfidl-parser.cc \
  sfidl-factory.cc sfidl-typelist.cc sfidl-cbase.cc sfidl-clientc.cc sfidl-clientcxx.cc sfidl-corec.cc \
  sfidl-corecxx.cc sfidl-cxxbase.cc sfidl-hostc.cc sfidl-utils.cc

bin_PROGRAMS = sfidl
sfidl_SOURCES = sfidl.cc $(common_idl_sources)
sfidl_LDADD = $(SFI_LIBS) -lm $(top_builddir)/birnet/libbirnet.o # libsfi.la
EXTRA_DIST += sfidl-generator.hh sfidl-namespace.hh sfidl-options.hh sfidl-parser.hh sfidl-factory.hh
EXTRA_DIST += sfidl-cbase.hh sfidl-clientc.hh sfidl-clientcxx.hh sfidl-cxxbase.hh sfidl-hostc.hh sfidl-utils.hh

EXTRA_DIST += $(sfi_public_headers) $(sfi_extra_sources) $(sfi_built_sources)
MAINTAINERCLEANFILES += $(sfi_built_sources)
