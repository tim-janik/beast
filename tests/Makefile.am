# BSE - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 2003 Tim Janik
#
## Makefile.am for BEAST tests

SUBDIRS = audio latency scripts

CFLAGS   += $(EXTRA_CFLAGS)
CPPFLAGS += $(EXTRA_CXXFLAGS)
INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)

DEFS += -DG_LOG_DOMAIN="\"BEAST-TEST\"" # -DG_DISABLE_CONST_RETURNS

SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)

#
# rules to generate built sources
#
# setup autogeneration dependancies
# 
gen_sources = xgen-bcah xgen-bcac
CLEANFILES += $(gen_sources)

# initial creation of the real stamp-* files
# normal autogeneration rules
bsecxxapi.h: $(top_srcdir)/bse/bse.idl $(SFIDL)
	cd . \
	&& echo -e "#include <sfi/sficxx.h>\n" > xgen-bcah \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --header $(top_srcdir)/bse/bse.idl >> xgen-bcah \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --header $(srcdir)/testplugin.idl >> xgen-bcah \
	&& cp xgen-bcah $@ \
	&& rm -f xgen-bcah
bsecxxapi.cc: bsecxxapi.h $(top_srcdir)/bse/bse.idl $(SFIDL)
	cd . \
	&& echo -e "#include \"bsecxxapi.h\"\n" > xgen-bcac \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --source $(top_srcdir)/bse/bse.idl >> xgen-bcac \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --source $(srcdir)/testplugin.idl >> xgen-bcac \
	&& cp xgen-bcac $@ \
	&& rm -f xgen-bcac
CLEANFILES += bsecxxapi.h bsecxxapi.cc

# source to built, that don't get installed
tests_built_sources = @STRIP_BEGIN@ \
	bsecxxapi.cc \
	bsecxxapi.h \
@STRIP_END@

test:
	$(MAKE) -C audio/ $(AM_MAKEFLAGS) $@

#
# convenience targets for the builtsources
#
.PHONY: generated clean-generated test
clean-generated:
	cd $(srcdir) \
	&& rm -f $(tests_built_sources)
	rm -f $(tests_built_sources)
generated: $(tests_built_sources)

#
# the test programs:
#

TESTS = cxxbinding perftest
progs_ldadd = $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la
cxxbinding_SOURCES = cxxbinding.cc bsecxxapi.cc
cxxbinding_LDADD = $(progs_ldadd)
$(srcdir)/cxxbinding.cc: bsecxxapi.h
EXTRA_DIST += empty.ogg

perftest_SOURCES = perftest.cc bsecxxapi.cc
perftest_LDADD = $(progs_ldadd)
$(srcdir)/perftest.cc: bsecxxapi.h

noinst_PROGRAMS = $(TESTS)

# test plugin: setup generation of C++ plugins from idl files
%.genidl.hh: %.idl $(BSEPGEN) $(SFIDL)
	$(SFIDL) --core-cxx $(SFIDL_INC) $< > $@ || (rm $@ ; exit 1 )

# test plugin: library
testplugin_la_SOURCES = testplugin.cc
testplugin_la_LDFLAGS = -module -avoid-version
testplugin_la_LIBADD  = $(BSE_LIBS) -lm
EXTRA_HEADERS += testplugin.idl
$(srcdir)/testplugin.cc: testplugin.idl

lib_LTLIBRARIES = testplugin.la
# Prevent those libs from being installed
install-libLTLIBRARIES:

$(srcdir)/testplugin.cc: testplugin.genidl.hh

CLEANFILES += testplugin.genidl.hh
EXTRA_DIST += $(strip \
	testplugin.genidl.hh 	\
	$(EXTRA_HEADERS)	\
)
