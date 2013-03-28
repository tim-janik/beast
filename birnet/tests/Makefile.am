# Birnet
# Copyright (C) 2006 Tim Janik
include $(top_srcdir)/Makefile.decl

INCLUDES    += -I$(top_srcdir) -I$(top_builddir) -I.
DEFS	    += @DEFINE__FILE_DIR__@ -DBIRNET_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS  = $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE $(BIRNET_CFLAGS)

noinst_PROGRAMS  = $(ALLTESTS)
progs_ldadd 	 = $(top_builddir)/birnet/libbirnet.o $(RAPICORN_LIBS) $(BIRNET_LIBS) -lm

TESTS		+= math
PERFTESTS       += math
math_SOURCES	 = math.cc
math_LDADD	 = $(progs_ldadd)
TESTS		+= strings
SLOWTESTS       += strings
strings_SOURCES  = strings.cc
strings_LDADD	 = $(progs_ldadd)
TESTS		+= systest
systest_SOURCES  = systest.cc
systest_LDADD	 = $(progs_ldadd)
TESTS		+= datalist
datalist_SOURCES = datalist.cc
datalist_LDADD	 = $(progs_ldadd)
TESTS		+= utils
utils_SOURCES    = utils.cc
utils_LDADD	 = $(progs_ldadd)
