# BEAST & BSE
include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)

DEFS        += -DG_LOG_DOMAIN=\"SFI\" -DG_DISABLE_CONST_RETURNS -DPARANOID # -DG_DISABLE_CONST_RETURNS
AM_CPPFLAGS += -I$(top_srcdir)

# programs to build
#
# source files

noinst_PROGRAMS  = $(ALLTESTS)

noinst_PROGRAMS += sfidl
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

# testsfidl
TAPTESTS          += testsfidl
testsfidl_CXXFLAGS = $(AM_CXXFLAGS) $(SFI_CFLAGS) -O0
testsfidl_LDADD    = $(BSE_LIBS)
testsfidl_SOURCES  = testsfidl.cc
