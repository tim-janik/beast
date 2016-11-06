# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I.
DEFS	    += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(SFI_CPPFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE

noinst_PROGRAMS   = $(ALLTESTS)
progs_nosfi_ldadd = $(LIBBSE_LIBS)
progs_ldadd 	  = $(top_builddir)/sfi/libsfi-@MAJOR@.la $(progs_nosfi_ldadd)
SFIDL 		  = $(top_builddir)/sfi/sfidl

# ring
TAPTESTS    += ring
ring_SOURCES = ring.cc
ring_LDADD   = $(progs_ldadd)

# misctests
TAPTESTS          += misctests
misctests_SOURCES  = misctests.cc
misctests_LDADD	   = $(progs_ldadd)
$(srcdir)/misctests.cc: testidl.h testidl.c

# testcxx
TAPTESTS	       += testcxx
testcxx_SOURCES = testcxx.cc
testcxx_LDADD   = $(progs_ldadd)
$(srcdir)/testcxx.cc: testidl.h testidl.c

# testsfidl
TAPTESTS         += testsfidl
testsfidl_SOURCES = testsfidl.cc
testsfidl_LDADD   = $(progs_nosfi_ldadd)
$(srcdir)/testsfidl.cc: testidl.h testidl.c

# testidl (generation test)
EXTRA_DIST += testidl.idl
CLEANFILES += testidl.h testidl.c
testidl.h: $(srcdir)/testidl.idl $(SFIDL)
	$(SFIDL) --host-c --header $(srcdir)/testidl.idl > xgen-$(@F) \
	&& mv xgen-$(@F) $@
testidl.c: $(srcdir)/testidl.idl $(SFIDL)
	$(SFIDL) --host-c --source --init test_types_init $(srcdir)/testidl.idl > xgen-$(@F) \
	&& mv xgen-$(@F) $@
