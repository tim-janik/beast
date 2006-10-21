# beast/tests/bse
# Copyright (C) 2003-2006 Tim Janik
# Copyright (C) 2005 Stefan Westerfeld
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)
DEFS     += -DG_LOG_DOMAIN="\"BSE-TEST\"" # -DG_DISABLE_CONST_RETURNS
SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)

#
# rules to generate built sources
#
# rule for IDL files
%.genidl.hh: %.idl $(SFIDL)
	$(SFIDL) --core-cxx $(SFIDL_INC) -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )
# bsecxxapi.hh
GENERATED_CLEANFILES += bsecxxapi.hh
bsecxxapi.hh: $(top_srcdir)/bse/bse.idl $(SFIDL)
	cd . \
	&& echo -e "#include <sfi/sficxx.hh>\n" > xgen-$(@F) \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --header $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --header $(srcdir)/testplugin.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bsecxxapi.cc
GENERATED_CLEANFILES += bsecxxapi.cc
bsecxxapi.cc: bsecxxapi.hh $(top_srcdir)/bse/bse.idl $(SFIDL)
	cd . \
	&& echo -e "#include \"bsecxxapi.hh\"\n" > xgen-$(@F) \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --source $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --source $(srcdir)/testplugin.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)

#
# convenience targets for generated source files
#
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(GENERATED) stamp-*
generated: clean-generated $(GENERATED)


#
# test programs
#
noinst_PROGRAMS = $(TESTS)
progs_ldadd = $(top_builddir)/bse/libbse.la

SLOWTESTS         += cxxbinding
cxxbinding_SOURCES = cxxbinding.cc bsecxxapi.cc
cxxbinding_LDADD   = $(progs_ldadd)
$(srcdir)/cxxbinding.cc: bsecxxapi.hh
$(srcdir)/cxxbinding.cc: bsecxxapi.cc

TESTS             += filtertest
filtertest_SOURCES = filtertest.cc
filtertest_LDADD   = $(progs_ldadd)
EXTRA_DIST += empty.ogg

#
# test plugin library
#
lib_LTLIBRARIES = testplugin.la
testplugin_la_SOURCES = testplugin.cc
testplugin_la_LDFLAGS = -module -avoid-version
testplugin_la_LIBADD  = $(top_builddir)/bse/libbse.la $(BSE_LIBS) -lm
EXTRA_DIST += testplugin.idl
GENERATED_CLEANFILES += testplugin.genidl.hh
$(srcdir)/testplugin.cc: testplugin.genidl.hh
$(srcdir)/testplugin.cc: testplugin.idl
# prevent libs from being installed
install-libLTLIBRARIES:
