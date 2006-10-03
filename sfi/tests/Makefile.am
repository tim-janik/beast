# Birnet
# Copyright (C) 2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES       += -I$(top_srcdir) -I$(top_builddir) -I. $(SFI_CFLAGS)
DEFS	       += -DG_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID # -DG_DISABLE_CONST_RETURNS

TESTS 		 =
noinst_PROGRAMS  = $(TESTS)
progs_ldadd 	 = $(top_builddir)/birnet/libbirnet.o $(top_builddir)/sfi/libsfi.o $(SFI_LIBS) -lm

# ring
TESTS       += ring
ring_SOURCES = ring.c dummy.cc
ring_LDADD   = $(progs_ldadd)

# testcxx
TESTS	       += testcxx
testcxx_SOURCES = testcxx.cc
testcxx_LDADD   = $(progs_ldadd)
$(srcdir)/testcxx.cc: ../testidl.h ../testidl.c
