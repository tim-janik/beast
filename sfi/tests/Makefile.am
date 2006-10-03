# Birnet
# Copyright (C) 2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES       += -I$(top_srcdir) -I$(top_builddir) -I. $(SFI_CFLAGS)
DEFS	       += -DG_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID # -DG_DISABLE_CONST_RETURNS

TESTS 		  =
noinst_PROGRAMS   = $(TESTS)
progs_nosfi_ldadd = $(top_builddir)/birnet/libbirnet.o $(SFI_LIBS) -lm
progs_ldadd 	  = $(top_builddir)/birnet/libbirnet.o $(top_builddir)/sfi/libsfi.o $(SFI_LIBS) -lm
SFIDL 		  = $(top_builddir)/sfi/sfidl

# ring
TESTS       += ring
ring_SOURCES = ring.c dummy.cc
ring_LDADD   = $(progs_ldadd)

# misctests
TESTS	          += misctests
misctests_SOURCES  = misctests.c dummy.cc
misctests_LDADD	   = $(progs_ldadd)
$(srcdir)/misctests.c: testidl.h testidl.c

# testcxx
TESTS	       += testcxx
testcxx_SOURCES = testcxx.cc
testcxx_LDADD   = $(progs_ldadd)
$(srcdir)/testcxx.cc: testidl.h testidl.c

# testsfidl
TESTS	         += testsfidl
testsfidl_SOURCES = testsfidl.cc \
			../sfidl-generator.cc ../sfidl-namespace.cc ../sfidl-options.cc ../sfidl-parser.cc \
			../sfidl-factory.cc ../sfidl-typelist.cc ../sfidl-cbase.cc ../sfidl-clientc.cc \
			../sfidl-clientcxx.cc ../sfidl-corec.cc ../sfidl-corecxx.cc ../sfidl-cxxbase.cc \
			../sfidl-hostc.cc ../glib-extra.c
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
