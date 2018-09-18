# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I.
DEFS	    += -DG_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(SFI_CPPFLAGS) $(RAPICORN_CFLAGS)

noinst_PROGRAMS   = $(ALLTESTS)
SFIDL 		  = $(top_builddir)/sfi/sfidl

# testsfidl
TAPTESTS          += testsfidl
testsfidl_CXXFLAGS = $(AM_CXXFLAGS) $(SFI_CFLAGS) -O0
testsfidl_LDADD    = $(BSE_LIBS)
testsfidl_SOURCES  = testsfidl.cc

