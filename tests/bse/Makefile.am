# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN="\"BSE-TEST\"" # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(SFI_CPPFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE

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
noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd = $(top_builddir)/bse/libbse-@MAJOR@.la $(RAPICORN_LIBS) $(BSE_LIBS)

#SLOWTESTS         += cxxbinding
#cxxbinding_SOURCES = cxxbinding.cc bsecxxapi.cc
#cxxbinding_LDADD   = $(progs_ldadd)
#$(srcdir)/cxxbinding.cc: bsecxxapi.hh
#$(srcdir)/cxxbinding.cc: bsecxxapi.cc

TESTS             += filtertest
filtertest_SOURCES = filtertest.cc
filtertest_LDADD   = $(progs_ldadd)
EXTRA_DIST += empty.ogg

SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)

# test plugin library
lib_LTLIBRARIES = testplugin.la
testplugin_la_SOURCES = testplugin.cc
testplugin_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
testplugin_la_LIBADD  = $(top_builddir)/bse/libbse-@MAJOR@.la $(RAPICORN_LIBS) $(BSE_LIBS) -lm
EXTRA_DIST += testplugin.idl
testplugin.genidl.hh: testplugin.idl $(SFIDL)
	$(SFIDL) --core-cxx $(SFIDL_INC) -I$(top_builddir) $< > xgen-$(@F)
	mv xgen-$(@F) $@
GENERATED_CLEANFILES += testplugin.genidl.hh
$(srcdir)/testplugin.cc: testplugin.genidl.hh
$(srcdir)/testplugin.cc: testplugin.idl

# prevent libs from being installed
install-libLTLIBRARIES:
