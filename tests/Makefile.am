# BSE - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 2003 Tim Janik
#
## Makefile.am for BEAST tests

SUBDIRS =

INCLUDES += -I$(top_srcdir) -I$(top_builddir) $(SFI_CFLAGS)

DEFS += -DG_LOG_DOMAIN=\"BEAST-TEST\" # -DG_DISABLE_CONST_RETURNS

SFIDL = $(top_builddir)/sfi/sfidl

#
# rules to generate built sources
#
# setup autogeneration dependancies
# 
gen_sources = xgen-bcah xgen-bcac
CLEANFILES += $(gen_sources)

# initial creation of the real stamp-* files
# normal autogeneration rules
bsecxxapi.h: $(top_builddir)/bse/bse.idl $(SFIDL)
	cd . \
	&& echo -e "#include <bse/bsecxxsmart.h>\n" > xgen-bcah \
	&& echo -e "#include <string>\n" >> xgen-bcah \
	&& echo -e "extern inline std::string sfi_value_get_cxxstring(GValue*v) { return std::string (sfi_value_get_string (v)); }" >> xgen-bcah \
	&& $(SFIDL) --cxx --header --interface --prefix test_ $(top_builddir)/bse/bse.idl >> xgen-bcah \
	&& cp xgen-bcah $@ \
	&& rm -f xgen-bcah
bsecxxapi.cc: bsecxxapi.h $(top_builddir)/bse/bse.idl $(SFIDL)
	cd . \
	&& echo -e "#include \"bsecxxapi.h\"\n" > xgen-bcac \
	&& $(SFIDL) --cxx --source --interface --prefix test_ $(top_builddir)/bse/bse.idl >> xgen-bcac \
	&& cp xgen-bcac $@ \
	&& rm -f xgen-bcac
CLEANFILES += bsecxxapi.h bsecxxapi.cc

# source to built, that don't get installed
tests_built_sources = @STRIP_BEGIN@ \
	bsecxxapi.cc \
	bsecxxapi.h \
@STRIP_END@

#
# convenience targets for the builtsources
#
.PHONY: generated clean-generated
clean-generated:
	cd $(srcdir) \
	&& rm -f $(tests_built_sources)
	rm -f $(tests_built_sources)
generated: $(tests_built_sources)

#
# the test programs:
#
TESTS = cxxbinding
progs_ldadd = $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la
cxxbinding_SOURCES = cxxbinding.cc bsecxxapi.cc
cxxbinding_LDADD = $(progs_ldadd)
$(srcdir)/cxxbinding.cc: bsecxxapi.h

noinst_PROGRAMS = $(TESTS)

EXTRA_DIST += $(strip \
)
