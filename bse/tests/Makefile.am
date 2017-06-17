# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS =

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir)
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN=\"BSE-TEST\" # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(BSE_CFLAGS) $(SFI_CPPFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE

noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd     = ../libbse-@MAJOR@.la $(LIBBSE_LIBS)

TAPTESTS         += misctests
misctests_SOURCES = misctests.cc
misctests_LDADD   = $(progs_ldadd)

TAPTESTS          += filtertest
SLOWTESTS         += filtertest
filtertest_SOURCES = filtertest.cc
filtertest_LDADD   = $(progs_ldadd)

TAPTESTS       += testfft
testfft_SOURCES = testfft.cc
testfft_LDADD   = $(progs_ldadd)

TAPTESTS       += testcxx
testcxx_SOURCES = testcxx.cc
testcxx_LDADD   = $(progs_ldadd)

TAPTESTS           += subnormals
subnormals_SOURCES  = subnormals.cc subnormals-aux.cc
subnormals_LDADD    = $(progs_ldadd)
subnormals_CXXFLAGS = $(AM_CXXFLAGS) -ffast-math

TAPTESTS          += loophandle
loophandle_SOURCES = loophandle.cc
loophandle_LDADD   = $(progs_ldadd)

TAPTESTS          += blocktests
blocktests_SOURCES = blocktests.cc
blocktests_LDADD   = $(progs_ldadd)

TAPTESTS              += resamplehandle
SLOWTESTS             += resamplehandle
resamplehandle_SOURCES = resamplehandle.cc
resamplehandle_LDADD   = $(progs_ldadd)

TAPTESTS              += firhandle
firhandle_SOURCES      = firhandle.cc
firhandle_LDADD        = $(progs_ldadd)
