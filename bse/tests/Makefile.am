# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS =

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir)
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN=\"BSE-TEST\" # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(BSE_CFLAGS) $(SFI_CPPFLAGS) $(RAPICORN_CFLAGS)

noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd     = ../libbse-@MAJOR@.la $(BSE_LIBS)

TAPTESTS          += misctests
misctests_CXXFLAGS = $(BSE_CFLAGS) -DBSE_CONVENIENCE
misctests_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
misctests_SOURCES  = misctests.cc

TAPTESTS           += filtertest
SLOWTESTS          += filtertest
filtertest_CXXFLAGS = $(BSE_CFLAGS) -DBSE_CONVENIENCE
filtertest_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
filtertest_SOURCES  = filtertest.cc

TAPTESTS        += testfft
testfft_CXXFLAGS = $(BSE_CFLAGS) -DBSE_CONVENIENCE
testfft_LDADD    = ../libbse-@MAJOR@.la
testfft_SOURCES  = testfft.cc

TAPTESTS        += testcxx
testcxx_CXXFLAGS = $(BSE_CFLAGS) -DBSE_CONVENIENCE
testcxx_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
testcxx_SOURCES  = testcxx.cc

TAPTESTS           += subnormals
subnormals_CXXFLAGS = $(BSE_CFLAGS) -DBSE_CONVENIENCE -ffast-math
subnormals_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
subnormals_SOURCES  = subnormals.cc subnormals-aux.cc

TAPTESTS           += loophandle
loophandle_CXXFLAGS = $(BSE_CFLAGS) -DBSE_CONVENIENCE
loophandle_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
loophandle_SOURCES  = loophandle.cc

TAPTESTS           += blocktests
blocktests_CXXFLAGS = $(BSE_CFLAGS) -DBSE_CONVENIENCE
blocktests_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
blocktests_SOURCES  = blocktests.cc

TAPTESTS               += resamplehandle
SLOWTESTS              += resamplehandle
resamplehandle_CXXFLAGS = $(BSE_CFLAGS) -DBSE_CONVENIENCE
resamplehandle_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
resamplehandle_SOURCES  = resamplehandle.cc

TAPTESTS          += firhandle
firhandle_CXXFLAGS = $(BSE_CFLAGS) -DBSE_CONVENIENCE
firhandle_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
firhandle_SOURCES  = firhandle.cc
