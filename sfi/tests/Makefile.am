# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I.
DEFS	    += -D__TOPDIR__="\"$(abs_top_srcdir)/\""
DEFS	    += -DG_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(SFI_CPPFLAGS) $(RAPICORN_CFLAGS)

noinst_PROGRAMS   = $(ALLTESTS)
SFIDL 		  = $(top_builddir)/sfi/sfidl

# misctests
#TAPTESTS          += misctests
misctests_CXXFLAGS = $(AM_CXXFLAGS) $(SFI_CFLAGS)
misctests_LDADD	   = $(top_builddir)/sfi/libsfi-@MAJOR@.la $(LIBZ_LIBS)
misctests_SOURCES  = misctests.cc

# testsfidl
TAPTESTS          += testsfidl
testsfidl_CXXFLAGS = $(AM_CXXFLAGS) $(SFI_CFLAGS) -O0
testsfidl_LDADD    = $(BSE_LIBS)
testsfidl_SOURCES  = testsfidl.cc

