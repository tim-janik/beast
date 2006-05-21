# beast/slowtests
# Copyright (C) 2003-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = audio filecheck

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)
DEFS += -DG_LOG_DOMAIN="\"BEAST-TEST\"" # -DG_DISABLE_CONST_RETURNS
SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)

# === rules to generate built sources ===
#
# bsecxxapi.hh
GENERATED_CLEANFILES += bsecxxapi.hh
$(srcdir)/perftest.cc: bsecxxapi.hh
bsecxxapi.hh: $(top_srcdir)/bse/bse.idl $(SFIDL)
	cd . \
	&& echo -e "#include <sfi/sficxx.hh>\n" > xgen-$(@F) \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --header $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bsecxxapi.cc
GENERATED_CLEANFILES += bsecxxapi.cc
$(srcdir)/perftest.cc: bsecxxapi.cc
bsecxxapi.cc: bsecxxapi.hh $(top_srcdir)/bse/bse.idl $(SFIDL)
	cd . \
	&& echo -e "#include \"bsecxxapi.hh\"\n" > xgen-$(@F) \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --source $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)


# === generation targets ===
# convenience targets for generated source files
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(GENERATED) stamp-*
generated: clean-generated $(GENERATED)


# === test programs ===
TESTS = perftest testwavechunk
noinst_PROGRAMS = $(TESTS)
progs_ldadd = $(top_builddir)/bse/libbse.la
# testwavechunk
testwavechunk_SOURCES = testwavechunk.c cxxdummy.cc
testwavechunk_LDADD = $(progs_ldadd)
# perftest
perftest_SOURCES = perftest.cc bsecxxapi.cc
perftest_LDADD = $(progs_ldadd)
