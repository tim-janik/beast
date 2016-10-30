# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS      = . tests
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN=\"SFI\" -DG_DISABLE_CONST_RETURNS
AM_CPPFLAGS += -I$(top_srcdir)

sfi_public_headers = $(strip \
	sfistore.hh	sficomwire.hh	sfifilecrawler.hh \
	glib-extra.hh			sfiwrapper.hh	 \
	sfivmarshal.hh	sfiglue.hh	sfigluecodec.hh	sfiglueproxy.hh	\
	sfinote.hh	sfiparams.hh	sfiprimitives.hh	sfiserial.hh	\
	sfitime.hh	sfitypes.hh	sfivalues.hh	\
	sfivisitors.hh	sfiustore.hh	\
	sficxx.hh	sfiring.hh	sfimemory.hh	sficomport.hh	\
	sfitests.hh	sfi.hh						\
	gbsearcharray.hh \
)
sfi_all_sources = $(strip \
	sfistore.cc	sficomwire.cc	sfifilecrawler.cc \
	glib-extra.cc			sfiwrapper.cc	sfiprimitives.cc \
	sfivmarshal.cc	sfiglue.cc	sfigluecodec.cc	sfiglueproxy.cc	\
	sfinote.cc	sfiparams.cc			sfiserial.cc	\
	sfitime.cc	sfitypes.cc	sfivalues.cc	\
	sfivisitors.cc	sfiustore.cc	\
			sfiring.cc	sfimemory.cc	sficomport.cc	\
)
sfi_extra_sources = $(strip \
)
sfi_built_sources = $(strip \
)

$(sfi_all_sources): $(sfi_built_sources)

# == libsfi-@MAJOR@.so ==
noinst_LTLIBRARIES            = libsfi-@MAJOR@.la
libsfi_@MAJOR@includedir      = $(bseincludedir)/sfi
libsfi_@MAJOR@include_HEADERS = $(sfi_public_headers)
libsfi_@MAJOR@_la_SOURCES     = $(sfi_all_sources)
libsfi_@MAJOR@_la_CXXFLAGS    = $(AM_CXXFLAGS) $(LIBBSE_CFLAGS) -DRAPICORN_CONVENIENCE
libsfi_@MAJOR@_la_LIBADD      = $(LIBBSE_LIBS)

#
# programs to build
#
# source files

bin_PROGRAMS   = sfidl
sfidl_SOURCES  = sfidl.cc
sfidl_CXXFLAGS = $(AM_CXXFLAGS) $(LIBBSE_CFLAGS) -DRAPICORN_CONVENIENCE
sfidl_LDADD    = $(LIBBSE_LIBS)
# included files
EXTRA_DIST += sfidl-generator.cc sfidl-namespace.cc sfidl-options.cc sfidl-parser.cc sfidl-factory.cc
EXTRA_DIST += sfidl-typelist.cc sfidl-cbase.cc sfidl-clientc.cc sfidl-clientcxx.cc sfidl-corec.cc
EXTRA_DIST += sfidl-corecxx.cc sfidl-cxxbase.cc sfidl-hostc.cc sfidl-utils.cc
# headers
EXTRA_DIST += sfidl-generator.hh sfidl-namespace.hh sfidl-options.hh sfidl-parser.hh sfidl-factory.hh
EXTRA_DIST += sfidl-cbase.hh sfidl-clientc.hh sfidl-clientcxx.hh sfidl-cxxbase.hh sfidl-hostc.hh sfidl-utils.hh

EXTRA_DIST += $(sfi_public_headers) $(sfi_extra_sources) $(sfi_built_sources)
MAINTAINERCLEANFILES += $(sfi_built_sources)
