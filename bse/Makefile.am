# BSE - Bedevilled Sound Engine
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BSE


INCLUDES = -I$(top_srcdir) $(BSE_CFLAGS)

#
# libraries to compile and install
#
lib_LTLIBRARIES = libbse.la

# due to automake-1.3 not featuring target specific defines,
# the G_LOG_DOMAIN needs to be special cased, depending on
# the source file sets ;(
DEFS += $(strip \
	$(patsubst %, -DBSE_COMPILATION -DG_LOG_DOMAIN=\"BSE\", \
	              $(filter $(notdir $<), $(bse_c_sources))) \
	$(patsubst %, -DBSE_COMPILATION -DG_LOG_DOMAIN=\"BSE-BUILTIN\", \
	              $(filter $(notdir $<), $(bse_builtin_c_sources))) \
)

# libtool stuff, set version, and export symbols for resolving
libbseincludedir = $(includedir)/bse
libbse_la_LDFLAGS = $(strp \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic \
)
libbse_la_LIBADD = $(BSE_LIBS) -lm


#
# setup source file variables
#
# header files for builtin plugins
bse_builtin_h_sources = $(strip \
)
# c sources for builtin plugins
bse_builtin_c_sources = $(strip \
	builtin_bsepattern.c \
	builtin_helloworld.c \
)
# BSE header files for public installation
bse_public_h_sources = $(strip \
	bse.h \
	bsebindata.h \
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
	bsefilestream.h \
	bseglobals.h \
	bseinstrument.h \
	bseitem.h \
	bsemagic.h \
	bsemain.h \
	bsemaster.h \
	bsenullstream.h \
	bseobject.h \
	bseparam.h \
	bsepattern.h \
	bsepcmstream.h \
	bsepcmstream-oss.h \
	bseplugin.h \
	bseprocedure.h \
	bseproject.h \
	bsesample.h \
	bsesong.h \
	bsesongsequencer.h \
	bsesource.h \
	bsestorage.h \
	bsestream.h \
	bsesuper.h \
	bsetext.h \
	bsetype.h \
	bseutils.h \
	glib-extra.h \
)
# BSE header files that don't get installed
bse_private_h_sources = $(strip \
	bsevoice.h \
)
# BSE c sources to build the library from
bse_c_sources = $(strip \
	bsebindata.c \
	bsechunk.c \
	bsecontainer.c \
	bseeffect.c \
	bseeffectfinetune.c \
	bseeffectnotevolume.c \
	bseeffectpatternbreak.c \
	bseeffectpatternjump.c \
	bseeffectvolumedelta.c \
	bseenums.c \
	bsefilestream.c \
	bseglobals.c \
	bseinstrument.c \
	bseitem.c \
	bsemagic.c \
	bsemain.c \
	bsemaster.c \
	bsenullstream.c \
	bseobject.c \
	bseparam.c \
	bseparamcol.c \
	bsepattern.c \
	bsepcmstream.c \
	bsepcmstream-oss.c \
	bseplugin.c \
	bseprocedure.c \
	bseproject.c \
	bsesample.c \
	bsesong.c \
	bsesongsequencer.c \
	bsesource.c \
	bsestorage.c \
	bsestream.c \
	bsesuper.c \
	bsetext.c \
	bsetype.c \
	bseutils.c \
	bsevoice.c \
	glib-extra.c \
)
# we use our own built_sources variable rules instead of BUILT_SOURCES,
# since automake-1.3 handles BUILT_SOURCES strange and wrong on some
# occasions (mostly wrt dependancies)
# we generate frequently rebuild files piggyback on a stamp file, so sources
# depending on them only get rebuild when the built source actually changed
# content
#
# built sources that don't get installed
bse_built_sources = $(strip \
	stamp-bsegentypes.h \
	stamp-bseenum_arrays.c \
	bseenum_arrays.c \
	bseenum_list.c \
	bsenotifier_array.c \
	bsebuiltin_externs.c \
	bsebuiltin_array.c \
	bsegentypes.c \
	bsegentype_array.c \
)
# built sources that get installed with the header files
bse_built_public_sources = $(strip \
	bsegentypes.h \
	bseconfigpaths.h \
)
# non-header sources (headers should be specified in the above variables)
# that don't serve as direct make target sources, i.e. they don't have
# their own .lo rules and don't get publically installed
bse_extra_sources = $(strip \
	mkbuiltinlist.pl \
	mkenums.pl \
	mktypes.pl \
	bseconfig.h.in \
	bsesongmixer.c \
)

#
# setup BSE sources and their dependancies
#
bse_h_sources = $(bse_private_h_sources) $(bse_public_h_sources) $(bse_built_public_sources)
libbseinclude_HEADERS = $(bse_public_h_sources) $(bse_built_public_sources)
libbse_la_SOURCES = $(bse_c_sources) $(bse_builtin_c_sources)
MAINTAINERCLEANFILES += $(bse_built_public_sources) $(bse_built_sources)
EXTRA_DIST += $(bse_builtin_h_sources) $(bse_private_h_sources)
EXTRA_DIST += $(bse_built_sources) $(bse_built_public_sources) $(bse_extra_sources)

#
# rules to generate built sources
#
gen_source = gen-source.tmp
CLEANFILES += $(gen_source)
COPYING: $(bse_built_public_sources) $(bse_built_sources)
$(OBJECTS): COPYING # this is our oldest-source-stamp
bsegentypes.h: @MISSING_PERL5@ # stamp-bsegentypes.h
	-test -e "$@" || touch "$@"
stamp-bsegentypes.h: @MISSING_PERL5@ $(srcdir)/mkenums.pl $(srcdir)/mktypes.pl
stamp-bsegentypes.h: @MISSING_PERL5@ @BIRNET_N@ $(bse_c_sources)
stamp-bsegentypes.h: @MISSING_PERL5@ $(bse_h_sources)
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkenums.pl --externs $(bse_h_sources) > $(gen_source) \
	&& $(PERL) $(srcdir)/mktypes.pl --externs $(bse_c_sources) >> $(gen_source) \
	&& (cmp -s $(gen_source) bsegentypes.h || mv $(gen_source) bsegentypes.h) \
	&& rm -f $(gen_source) \
	&& echo timestamp > $@
bsegentypes.c: @MISSING_PERL5@ bsegentypes.h
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkenums.pl --interns $(bse_h_sources) > $(gen_source) \
	&& $(PERL) $(srcdir)/mktypes.pl --interns --export-proto $(bse_c_sources) >> $(gen_source) \
	&& mv $(gen_source) $@
bsegentype_array.c: @MISSING_PERL5@ bsegentypes.c
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mktypes.pl --array $(bse_c_sources) > $(gen_source) \
	&& mv $(gen_source) $@
stamp-bseenum_arrays.c: @MISSING_PERL5@ $(bse_h_sources) $(srcdir)/mkenums.pl
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkenums.pl --arrays --includes $(bse_h_sources) > $(gen_source) \
	&& (cmp -s $(gen_source) bseenum_arrays.c || mv $(gen_source) bseenum_arrays.c) \
	&& rm -f $(gen_source) \
	&& echo timestamp > $@
bseenum_list.c: @MISSING_PERL5@ bseenum_arrays.c $(srcdir)/mkenums.pl
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkenums.pl --list $(bse_h_sources) > $(gen_source) \
	&& mv $(gen_source) $@
bsebuiltin_externs.c: @MISSING_PERL5@ $(bse_builtin_c_sources) $(srcdir)/mkbuiltinlist.pl
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkbuiltinlist.pl --externs $(bse_builtin_c_sources) > $(gen_source) \
	&& mv $(gen_source) $@
bsebuiltin_array.c: @MISSING_PERL5@ $(bse_builtin_c_sources) $(srcdir)/mkbuiltinlist.pl
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mkbuiltinlist.pl --array $(bse_builtin_c_sources) > $(gen_source) \
	&& mv $(gen_source) $@
bsenotifier_array.c: bsedefs.h
	cd $(srcdir) \
	&& echo "/* Generated data from $< (by make $@) */" > $(gen_source) \
	&& echo >> $(gen_source) \
	&& $(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	          $(AM_CFLAGS) $(CFLAGS) -c $< \
	| sed -e'/BseNotify_/{' \
	  -e's/\(.*\)BseNotify_\([^)]*\).*(\([^	 ]*\).*/  { "\2", "\3", },/' \
	  -ep -e'}' -ed   >> $(gen_source) \
	&& echo >> $(gen_source) \
	&& echo "/* Generated data ends here */" >> $(gen_source) \
	&& mv $(gen_source) $@
bseconfigpaths.h: $(top_srcdir)/configure bseconfig.h Makefile
	cd $(srcdir) \
	&& echo "/* Generated data from $< (by make $@) */" > $(gen_source) \
	&& echo >> $(gen_source) \
	&& echo "#define BSE_PATH_INCLUDES \"${libbseincludedir}\"" >> $(gen_source) \
	&& echo "#define BSE_PATH_DATA     \"${bsedatadir}\"" >> $(gen_source) \
	&& echo "#define BSE_PATH_LIB      \"${bselibdir}\"" >> $(gen_source) \
	&& echo "#define BSE_PATH_PLUGINS  (BSE_PATH_LIB \"/plugins\")" >> $(gen_source) \
	&& if test -z '$(bseossdevice)' ; then \
	     echo '#undef BSE_PATH_DEVICE_OSS' >> $(gen_source) ; \
	   else \
	     echo '#define BSE_PATH_DEVICE_OSS $(bseossdevice)' >> $(gen_source) ; \
	   fi \
	&& if test -z '$(bsealsadevices)' ; then \
	     echo '#undef BSE_PATH_DEVICE_LIST_ALSA' >> $(gen_source) ; \
	   else \
	     echo '#define BSE_PATH_DEVICE_LIST_ALSA $(bsealsadevices)' >> $(gen_source) ; \
	   fi \
	&& echo >> $(gen_source) \
	&& echo "/* Generated data ends here */" >> $(gen_source) \
	&& mv $(gen_source) $@


#
# convenience targets for the builtsources
#
#.PHONY: builtsources clean-builtsources
#clean-builtsources:
#	rm -f $(bse_built_public_sources) $(bse_built_sources)
#builtsources:		$(bse_built_public_sources) $(bse_built_sources)


#
# private (birnet) dependancy to make TAGS file after each build
#
all: @MISSING_ETAGS@ @BIRNET_Y@ TAGS


#
# other programs, we want to compile
#
noinst_PROGRAMS = bsequery testtypes testsample testsong testmixer testrefs
# source files
testtypes_SOURCES = testtypes.c
testrefs_SOURCES = testrefs.c
testsample_SOURCES = testsample.c
bsequery_SOURCES = bsequery.c
testsong_SOURCES = testsong.c
testmixer_SOURCES = testmixer.c
# link programs against BSE
progs_LDADD = libbse.la
testtypes_LDADD = $(progs_LDADD)
testrefs_LDADD = $(progs_LDADD)
testsample_LDADD = $(progs_LDADD)
bsequery_LDADD = $(progs_LDADD)
testsong_LDADD = $(progs_LDADD)
testmixer_LDADD = $(progs_LDADD)


#
# auxillary files
#
EXTRA_DIST += \
	raw2bse \
	TODO
