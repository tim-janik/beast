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
gen_sources = xgen-bah xgen-bac
CLEANFILES += $(gen_sources)
:r
# initial creation of the real stamp-* files
bseapi.h: stamp-bseapi.h ;
# normal autogeneration rules
stamp-bseapi.h: $(top_builddir)/bse/bse.idl $(SFIDL)
	echo -e "#include <sfi/sfi.h>\n" > xgen-bah \
	&& $(SFIDL) --cxx --header --interface --prefix beast_ $(top_builddir)/bse/bse.idl >> xgen-bah \
	&& (cmp -s xgen-bah bseapi.h || cp xgen-bah bseapi.h) \
	&& rm -f xgen-bah \
	&& echo timestamp > $@
bseapi.cc: bseapi.h $(top_builddir)/bse/bse.idl $(SFIDL)
	echo -e "#include \"bseapi.h\"\n" > xgen-bac \
	&& $(SFIDL) --cxx --source --interface --prefix beast_ $(top_builddir)/bse/bse.idl >> xgen-bac \
	&& cp xgen-bac $@ \
	&& rm -f xgen-bac

# source to built, that don't get installed
tests_built_sources = @STRIP_BEGIN@ \
	stamp-bswgenapi.h \
	bseapi.cc \
	bseapi.h \
@STRIP_END@

#
# convenience targets for the builtsources
#
.PHONY: builtsources clean-builtsources
clean-builtsources:
	cd $(srcdir) \
	&& rm -f $(tests_built_sources)
	rm -f $(tests_built_sources)
builtsources: $(tests_built_sources)

#
# the test programs, finally:
#
TESTS = cxxbinding
progs_ldadd = $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la
cxxbinding_SOURCES = cxxbinding.cc bseapi.cc
cxxbinding_LDADD = $(progs_ldadd)

$(srcdir)/cxxbinding.cc: bseapi.h

noinst_PROGRAMS = $(TESTS)

EXTRA_DIST += $(strip \
)
