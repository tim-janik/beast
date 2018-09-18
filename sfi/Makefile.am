# BEAST & BSE
include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)

SUBDIRS      = . tests
DEFS        += -DG_LOG_DOMAIN=\"SFI\" -DG_DISABLE_CONST_RETURNS
AM_CPPFLAGS += -I$(top_srcdir)

sfi_public_headers = $(strip	\
	glib-extra.hh		\
)
sfi_all_sources = $(strip	\
	glib-extra.cc		\
)
sfi_extra_sources = $(strip		\
	private.hh			\
)

# == doc_sources.lst ==
doc_sources.lst: Makefile
	$(AM_V_GEN)
	$(Q) echo $(patsubst %, sfi/%, $(sfi_public_headers) $(sfi_all_sources)) > $@
CLEANFILES += doc_sources.lst

# == libsfi-@MAJOR@.so ==
#noinst_LTLIBRARIES            = libsfi-@MAJOR@.la
#libsfi_@MAJOR@includedir      = $(bseincludedir)/sfi
#libsfi_@MAJOR@include_HEADERS = $(sfi_public_headers) sysconfig.h
#libsfi_@MAJOR@_la_SOURCES     = $(sfi_all_sources)
#libsfi_@MAJOR@_la_CXXFLAGS    = $(AM_CXXFLAGS) $(SFI_CFLAGS)
#libsfi_@MAJOR@_la_LIBADD      = $(SFI_LIBS)
#$(sfi_public_headers):		sysconfig.h

#
# programs to build
#
# source files

noinst_PROGRAMS = sfidl
sfidl_SOURCES  = sfidl.cc
sfidl_CXXFLAGS = $(AM_CXXFLAGS) $(SFI_CFLAGS) -DSFIDL_INTERNALS -O0
sfidl_LDADD    = $(SFI_LIBS)
# included files
sfidl_includes  = sfidl-generator.cc sfidl-namespace.cc sfidl-options.cc sfidl-parser.cc sfidl-factory.cc
sfidl_includes += sfidl-typelist.cc sfidl-cbase.cc sfidl-clientc.cc sfidl-clientcxx.cc sfidl-corec.cc
sfidl_includes += sfidl-corecxx.cc sfidl-cxxbase.cc sfidl-hostc.cc sfidl-utils.cc
# headers
sfidl_includes += sfidl-generator.hh sfidl-namespace.hh sfidl-options.hh sfidl-parser.hh sfidl-factory.hh
sfidl_includes += sfidl-cbase.hh sfidl-clientc.hh sfidl-clientcxx.hh sfidl-cxxbase.hh sfidl-hostc.hh sfidl-utils.hh
