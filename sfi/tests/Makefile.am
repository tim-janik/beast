# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I.
DEFS	    += -D__TOPDIR__="\"$(abs_top_srcdir)/\""
DEFS	    += -DG_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(SFI_CPPFLAGS) $(RAPICORN_CFLAGS)

noinst_PROGRAMS   = $(ALLTESTS)
SFIDL 		  = $(top_builddir)/sfi/sfidl

# ring
TAPTESTS     += ring
ring_CXXFLAGS = $(AM_CXXFLAGS) $(SFI_CFLAGS)
ring_LDADD    = $(top_builddir)/sfi/libsfi-@MAJOR@.la
ring_SOURCES  = ring.cc

# misctests
TAPTESTS          += misctests
misctests_CXXFLAGS = $(AM_CXXFLAGS) $(SFI_CFLAGS)
misctests_LDADD	   = $(top_builddir)/sfi/libsfi-@MAJOR@.la $(LIBZ_LIBS)
misctests_SOURCES  = misctests.cc utf8tests.cc

# testcxx
TAPTESTS	+= testcxx
testcxx_CXXFLAGS = $(AM_CXXFLAGS) $(SFI_CFLAGS)
testcxx_LDADD	 = $(top_builddir)/sfi/libsfi-@MAJOR@.la
testcxx_SOURCES  = testcxx.cc

# testsfidl
TAPTESTS          += testsfidl
testsfidl_CXXFLAGS = $(AM_CXXFLAGS) $(SFI_CFLAGS)
testsfidl_LDADD    = $(top_builddir)/sfi/libsfi-@MAJOR@.la
testsfidl_SOURCES  = testsfidl.cc

# == t279-assertions-test ==
t279-assertions-test: misctests
	@echo "  RUN…    " $@
	$(Q) ./misctests --return_unless1 || $(QDIE) --return_unless1 failed
	$(Q) ./misctests --assert_return1 || $(QDIE) --assert_return1 failed
	$(Q) (trap ':' SIGTRAP && ./misctests --return_unless0) $(QSTDERR) ; test "$$?" -eq 7 || $(QDIE) --return_unless0 failed
	$(Q) (trap ':' SIGTRAP && ./misctests --assert_return0) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --assert_return0 failed
	$(Q) (trap ':' SIGTRAP && ./misctests --assert_return_unreached) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --assert_return_unreached failed
	$(Q) (trap ':' SIGTRAP && ./misctests --fatal_error) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --fatal_error failed
	$(Q) ./misctests --backtrace          2>t279-assertions-test.tmp && \
		grep -qi 'Backtrace'		t279-assertions-test.tmp && \
		grep -qi 'in.*my_compare_func'	t279-assertions-test.tmp || $(QDIE) --backtrace failed
	$(Q) rm -f t279-assertions-test.tmp
	@echo "  PASS    " $@
check-local: t279-assertions-test
