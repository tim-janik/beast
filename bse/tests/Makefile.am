# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS =

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir)
DEFS        += -DG_LOG_DOMAIN=\"BSE-TEST\" # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(BSE_CFLAGS) $(SFI_CPPFLAGS)

noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd     = ../libbse-@MAJOR@.la $(BSE_LIBS)

TAPTESTS	  += integrity
integrity_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS)
integrity_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
integrity_SOURCES  = integrity.cc

TAPTESTS          += misctests
misctests_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS)
misctests_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
misctests_SOURCES  = misctests.cc

TAPTESTS           += filtertest
SLOWTESTS          += filtertest
filtertest_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS)
filtertest_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
filtertest_SOURCES  = filtertest.cc

TAPTESTS        += testfft
testfft_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS)
testfft_LDADD    = ../libbse-@MAJOR@.la
testfft_SOURCES  = testfft.cc

TAPTESTS           += subnormals
subnormals_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS) -ffast-math
subnormals_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
subnormals_SOURCES  = subnormals.cc subnormals-aux.cc

TAPTESTS           += loophandle
loophandle_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS)
loophandle_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
loophandle_SOURCES  = loophandle.cc

TAPTESTS           += blocktests
blocktests_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS)
blocktests_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
blocktests_SOURCES  = blocktests.cc

TAPTESTS               += resamplehandle
SLOWTESTS              += resamplehandle
resamplehandle_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS)
resamplehandle_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
resamplehandle_SOURCES  = resamplehandle.cc

TAPTESTS          += firhandle
firhandle_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS)
firhandle_LDADD    = ../libbse-@MAJOR@.la $(SFI_LIBS)
firhandle_SOURCES  = firhandle.cc

# == t279-assertions-test ==
t279-assertions-test: integrity
	@echo "  RUN…    " $@
	$(Q) ./integrity --return_unless1 || $(QDIE) --return_unless1 failed
	$(Q) ./integrity --assert_return1 || $(QDIE) --assert_return1 failed
	$(Q) (trap ':' SIGTRAP && ./integrity --return_unless0) $(QSTDERR) ; test "$$?" -eq 7 || $(QDIE) --return_unless0 failed
	$(Q) (trap ':' SIGTRAP && ./integrity --assert_return0) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --assert_return0 failed
	$(Q) (trap ':' SIGTRAP && ./integrity --assert_return_unreached) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --assert_return_unreached failed
	$(Q) (trap ':' SIGTRAP && ./integrity --fatal_error) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --fatal_error failed
	$(Q) ./integrity --backtrace          2>t279-assertions-test.tmp && \
		grep -qi 'Backtrace'		t279-assertions-test.tmp && \
		grep -qi 'in.*my_compare_func'	t279-assertions-test.tmp || $(QDIE) --backtrace failed
	$(Q) rm -f t279-assertions-test.tmp
	@echo "  PASS    " $@
check-local: t279-assertions-test
