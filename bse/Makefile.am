# BSE - Bedevilled Sound Engine
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BSE

SUBDIRS = icons

INCLUDES = -I$(top_srcdir) $(BSE_CFLAGS)

#
# libraries to compile and install
#
lib_LTLIBRARIES = libbse.la

# due to automake-1.3 not featuring target specific defines,
# the G_LOG_DOMAIN needs to be special cased, depending on
# the source file sets ;(
DEFS += @STRIP_BEGIN@ \
	$(patsubst %, -DG_LOG_DOMAIN=bse_log_domain_bse -DBSE_COMPILATION, \
	              $(filter $(<F), $(bse_c_sources))) \
	$(patsubst %, -DBSE_BUILTIN_NAME=$(strip $(subst -,_,$(basename $(<F)))) \
                      -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\"" \
	              -DG_LOG_DOMAIN=BSE_PLUGIN_NAME -DBSE_COMPILATION, \
	              $(filter $(<F), $(bse_builtin_c_sources))) \
@STRIP_END@

# libtool stuff: set version and export symbols for resolving
libbseincludedir = $(includedir)/bse
libbse_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic \
@STRIP_END@
libbse_la_LIBADD = $(BSE_LIBS) -lm


#
# setup source file variables
#
# header files for builtin plugins
bse_builtin_h_sources = @STRIP_BEGIN@ \
@STRIP_END@
# c sources for builtin plugins
bse_builtin_c_sources = @STRIP_BEGIN@ \
	builtin_bsepattern.c \
	builtin_helloworld.c \
@STRIP_END@
# BSE header files for public installation (non-generated)
bse_public_h_sources = @STRIP_BEGIN@ \
	bse.h \
	bsebindata.h \
	bsecategories.h \
	bsechunk.h \
	bseconfig.h \
	bsecontainer.h \
	bsedefs.h \
	bseeffect.h \
	bseeffectfinetune.h \
	bseeffectnotevolume.h \
	bseeffectpatternbreak.h \
	bseeffectpatternjump.h \
	bseeffectvolumedelta.h \
	bseenums.h \
	bseexports.h \
	bsegconfig.h \
	bseglobals.h \
	bseheart.h \
	bseinstrument.h \
	bseitem.h \
	bsemagic.h \
	bsemain.h \
	bsemixer.h \
	bseobject.h \
	bseparam.h \
	bseparasite.h \
	bsepattern.h \
	bsepcmdevice.h \
	bsepcmdevice-oss.h \
	bsepcmdevice-alsa.h \
	bseplugin.h \
	bseprocedure.h \
	bseproject.h \
	bsesample.h \
	bsesnet.h \
	bsesong.h \
	bsesongsequencer.h \
	bsesource.h \
	bsestorage.h \
	bsesuper.h \
	bsetype.h \
	bseutils.h \
	glib-extra.h \
@STRIP_END@
# BSE header files that don't get installed
bse_private_h_sources = @STRIP_BEGIN@ \
	bsevoice.h \
@STRIP_END@
# BSE C sources to build the library from
bse_c_sources = @STRIP_BEGIN@ \
	bsebindata.c \
	bsecategories.c \
	bsechunk.c \
	bsecontainer.c \
	bseeffect.c \
	bseeffectfinetune.c \
	bseeffectnotevolume.c \
	bseeffectpatternbreak.c \
	bseeffectpatternjump.c \
	bseeffectvolumedelta.c \
	bseenums.c \
	bsegconfig.c \
	bseglobals.c \
	bseheart.c \
	bseinstrument.c \
	bseitem.c \
	bsemagic.c \
	bsemain.c \
	bsemixer.c \
	bseobject.c \
	bseparam.c \
	bseparasite.c \
	bseparamcol.c \
	bsepattern.c \
	bsepcmdevice.c \
	bsepcmdevice-oss.c \
	bsepcmdevice-alsa.c \
	bseplugin.c \
	bseprocedure.c \
	bseproject.c \
	bsesample.c \
	bsesnet.c \
	bsesong.c \
	bsesongsequencer.c \
	bsesource.c \
	bsestorage.c \
	bsesuper.c \
	bsetype.c \
	bseutils.c \
	bsevoice.c \
	glib-extra.c \
@STRIP_END@
# we use our own built_sources variable rules instead of BUILT_SOURCES,
# since handles BUILT_SOURCES strange and wrong on some occasions (mostly
# wrt dependancies)
# we generate frequently rebuild files piggyback on a stamp file, so sources
# depending on them only get rebuild when the built source actually changed
# content
#
# built sources that don't get installed
bse_built_sources = @STRIP_BEGIN@ \
	stamp-bsegentypes.h \
	stamp-bseenum_arrays.c \
	bseenum_arrays.c \
	bseenum_list.c \
	bsenotifier_array.c \
	bsebuiltin_externs.c \
	bsebuiltin_array.c \
	bsegentypes.c \
	bsegentype_array.c \
@STRIP_END@
# built sources that get installed with the header files
bse_built_public_sources = @STRIP_BEGIN@ \
	bsegentypes.h \
@STRIP_END@
# non-header sources (headers should be specified in the above variables)
# that don't serve as direct make target sources, i.e. they don't have
# their own .lo rules and don't get publically installed
bse_extra_sources = @STRIP_BEGIN@ \
	mkbuiltinlist.pl \
	mkenums.pl \
	mktypes.pl \
	bseconfig.h.in \
	bsesongmixer.c \
@STRIP_END@

#
# setup BSE sources and their dependancies
#
bse_h_sources = $(bse_private_h_sources) $(bse_public_h_sources) $(bse_built_public_sources)
libbseinclude_HEADERS = $(bse_public_h_sources) $(bse_built_public_sources)
libbse_la_SOURCES = $(bse_c_sources) $(bse_builtin_c_sources)
MAINTAINERCLEANFILES += $(bse_built_public_sources) $(bse_built_sources)
EXTRA_HEADERS +=
EXTRA_DIST += $(bse_builtin_h_sources) $(bse_private_h_sources)
EXTRA_DIST += $(bse_built_sources) $(bse_built_public_sources) $(bse_extra_sources)

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-sbgth xgen-bgtc xgen-bgtac xgen-sbeac xgen-belc xgen-bbec xgen-bbac xgen-bnac
CLEANFILES += $(gen_sources)
COPYING: $(bse_built_public_sources) $(bse_built_sources)
$(OBJECTS): COPYING # this is our oldest-source-stamp
# initial creation of the real stamp-* files
bsegentypes.h: @MISSING_PERL5@		# never add deps here
	-test -f "$@" || touch "$@"
# normal autogeneration rules
stamp-bsegentypes.h: @MISSING_PERL5@ $(srcdir)/mkenums.pl $(srcdir)/mktypes.pl
stamp-bsegentypes.h: @MISSING_PERL5@ @BIRNET_N@ $(bse_c_sources)
stamp-bsegentypes.h: @MISSING_PERL5@ $(bse_h_sources)
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkenums.pl --externs $(bse_h_sources) > xgen-sbgth \
	&& $(PERL) $(srcdir)/mktypes.pl --externs $(bse_c_sources) >> xgen-sbgth \
	&& (cmp -s xgen-sbgth bsegentypes.h || cp xgen-sbgth bsegentypes.h) \
	&& rm -f xgen-sbgth \
	&& echo timestamp > $@
bsegentypes.c: @MISSING_PERL5@ bsegentypes.h
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkenums.pl --interns $(bse_h_sources) > xgen-bgtc \
	&& $(PERL) $(srcdir)/mktypes.pl --interns --export-proto $(bse_c_sources) >> xgen-bgtc \
	&& cp xgen-bgtc $@ \
	&& rm -f xgen-bgtc
bsegentype_array.c: @MISSING_PERL5@ bsegentypes.c
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mktypes.pl --array $(bse_c_sources) > xgen-bgtac \
	&& cp xgen-bgtac $@ \
	&& rm -f xgen-bgtac
stamp-bseenum_arrays.c: @MISSING_PERL5@ $(bse_h_sources) $(srcdir)/mkenums.pl
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkenums.pl --arrays --includes $(bse_h_sources) > xgen-sbeac \
	&& (cmp -s xgen-sbeac bseenum_arrays.c || cp xgen-sbeac bseenum_arrays.c) \
	&& rm -f xgen-sbeac \
	&& echo timestamp > $@
bseenum_list.c: @MISSING_PERL5@ bseenum_arrays.c $(srcdir)/mkenums.pl
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkenums.pl --list $(bse_h_sources) > xgen-belc \
	&& cp xgen-belc $@ \
	&& rm -f xgen-belc
bsebuiltin_externs.c: @MISSING_PERL5@ $(bse_builtin_c_sources) $(srcdir)/mkbuiltinlist.pl
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkbuiltinlist.pl --externs $(bse_builtin_c_sources) > xgen-bbec \
	&& cp xgen-bbec $@ \
	&& rm -f xgen-bbec
bsebuiltin_array.c: @MISSING_PERL5@ $(bse_builtin_c_sources) $(srcdir)/mkbuiltinlist.pl
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkbuiltinlist.pl --array $(bse_builtin_c_sources) > xgen-bbac \
	&& cp xgen-bbac $@ \
	&& rm -f xgen-bbac
bsenotifier_array.c: bsedefs.h
	cd $(srcdir) \
	&& echo "/* Generated data from $< (by make $@) */" > xgen-bnac \
	&& echo >> xgen-bnac \
	&& $(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	          $(AM_CFLAGS) $(CFLAGS) -c $< \
	| sed -e'/BseNotify_/{' \
	  -e's/\(.*\)BseNotify_\([^)]*\).*(\([^	 ]*\).*/  { "\2", "\3", },/' \
	  -ep -e'}' -ed   >> xgen-bnac \
	&& echo >> xgen-bnac \
	&& echo "/* Generated data ends here */" >> xgen-bnac \
	&& cp xgen-bnac $@ \
	&& rm -f xgen-bnac

#
# convenience targets for the builtsources
#
.PHONY: builtsources clean-builtsources
clean-builtsources:
	rm -f $(bse_built_public_sources) $(bse_built_sources)
builtsources:		$(bse_built_public_sources) $(bse_built_sources)


#
# private (birnet) dependancy to make TAGS file after each build
#
all: @MISSING_ETAGS@ @BIRNET_Y@ # TAGS


#
# other programs, we want to compile
#
noinst_PROGRAMS = bsequery testtypes testsample testsong testrefs
# source files
testtypes_SOURCES = testtypes.c
testrefs_SOURCES = testrefs.c
testsample_SOURCES = testsample.c
bsequery_SOURCES = bsequery.c
testsong_SOURCES = testsong.c
# link programs against BSE
progs_LDADD = libbse.la
testtypes_LDADD = $(progs_LDADD)
testrefs_LDADD = $(progs_LDADD)
testsample_LDADD = $(progs_LDADD)
bsequery_LDADD = $(progs_LDADD)
testsong_LDADD = $(progs_LDADD)


#
# auxillary files
#
EXTRA_DIST += \
	raw2bse \
	TODO
