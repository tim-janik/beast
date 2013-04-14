# Birnet
# Copyright (C) 2006 Tim Janik
include $(top_srcdir)/Makefile.decl

INCLUDES    += -I$(top_srcdir) -I$(top_builddir) -I.
DEFS	    += @DEFINE__FILE_DIR__@ -DBIRNET_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS  = $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE $(BIRNET_CFLAGS)

noinst_PROGRAMS  = $(ALLTESTS)
progs_ldadd 	 = $(RAPICORN_LIBS) $(BIRNET_LIBS) -lm

TESTS =
