# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I.
DEFS	    += -D__TOPDIR__="\"$(abs_top_srcdir)/\""
DEFS	    += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(SFI_CPPFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE

noinst_PROGRAMS   = $(ALLTESTS)
SFIDL 		  = $(top_builddir)/sfi/sfidl

# ring
TAPTESTS     += ring
ring_CXXFLAGS = $(SFI_CFLAGS) -DBSE_CONVENIENCE
ring_LDADD    = $(top_builddir)/sfi/libsfi-@MAJOR@.la
ring_SOURCES  = ring.cc

# misctests
TAPTESTS          += misctests
misctests_CXXFLAGS = $(SFI_CFLAGS) -DBSE_CONVENIENCE
misctests_LDADD	   = $(top_builddir)/sfi/libsfi-@MAJOR@.la
misctests_SOURCES  = misctests.cc
$(srcdir)/misctests.cc: testidl.h testidl.c

# testcxx
TAPTESTS	       += testcxx
testcxx_CXXFLAGS = $(SFI_CFLAGS) -DBSE_CONVENIENCE
testcxx_LDADD	   = $(top_builddir)/sfi/libsfi-@MAJOR@.la
testcxx_SOURCES = testcxx.cc
$(srcdir)/testcxx.cc: testidl.h testidl.c

# testsfidl
TAPTESTS          += testsfidl
testsfidl_CXXFLAGS = $(SFI_CFLAGS) -DBSE_CONVENIENCE
testsfidl_LDADD    = $(top_builddir)/sfi/libsfi-@MAJOR@.la
testsfidl_SOURCES  = testsfidl.cc
$(srcdir)/testsfidl.cc: testidl.h testidl.c

# testidl (generation test)
CLEANFILES += testidl.h testidl.c
testidl.h: $(srcdir)/testidl.idl $(SFIDL)
	$(SFIDL) --host-c --header $(srcdir)/testidl.idl > xgen-$(@F) \
	&& mv xgen-$(@F) $@
testidl.c: $(srcdir)/testidl.idl $(SFIDL)
	$(SFIDL) --host-c --source --init test_types_init $(srcdir)/testidl.idl > xgen-$(@F) \
	&& mv xgen-$(@F) $@

# == t279-assertions-test ==
t279-assertions-test: misctests
	@echo "  RUN…    " $@
	$(Q) ./misctests --return_unless1 || $(QDIE) --return_unless1 failed
	$(Q) ./misctests --assert_return1 || $(QDIE) --assert_return1 failed
	$(Q) (trap ':' SIGTRAP && ./misctests --return_unless0) $(QSTDERR) ; test "$?" != 0 || $(QDIE) --return_unless0 failed
	$(Q) (trap ':' SIGTRAP && ./misctests --assert_return0) $(QSTDERR) ; test "$?" != 0 || $(QDIE) --assert_return0 failed
	$(Q) (trap ':' SIGTRAP && ./misctests --assert_return_unreached) $(QSTDERR) ; test "$?" != 0 || $(QDIE) --assert_return_unreached failed
	$(Q) (trap ':' SIGTRAP && ./misctests --fatal_error) $(QSTDERR) ; test "$?" != 0 || $(QDIE) --fatal_error failed
	$(Q) ./misctests --backtrace          2>t279-assertions-test.tmp && \
		grep -qi 'Backtrace'		t279-assertions-test.tmp && \
		grep -qi 'in my_compare_func'	t279-assertions-test.tmp || $(QDIE) --backtrace failed
	$(Q) rm -f t279-assertions-test.tmp
	@echo "  PASS    " $@
check-local: t279-assertions-test
