# BSE - Bedevilled Sound Engine
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BSE

SUBDIRS = icons

INCLUDES = -I$(top_srcdir) $(BSE_CFLAGS) -DG_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS

#
# libraries to compile and install
#
lib_LTLIBRARIES = libbse.la

# due to automake-1.3 not featuring target specific defines,
# the G_LOG_DOMAIN needs to be special cased, depending on
# the source file sets ;(
DEFS += @STRIP_BEGIN@ \
	$(patsubst %, -DG_LOG_DOMAIN=\"BSE\" -DBSE_COMPILATION, \
	              $(filter $(<F), $(bse_c_sources))) \
	$(patsubst %, -DBSE_BUILTIN_NAME=$(strip $(subst -,_, $(subst .,_,$(basename $(<F))))) \
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
# BSE header files for public installation (non-generated)
bse_public_h_sources = @STRIP_BEGIN@ \
	bse.h \
	bseamplifier.h \
	bsebindata.h \
	bsebiquadfilter.h \
	bsecategories.h \
	bsecomwire.h \
	bseconfig.h \
	bseconstant.h \
	bsecontainer.h \
	bsedatapocket.h \
	bsedefs.h \
	bseeditablesample.h \
	bseeffect.h \
	bseeffectbalance.h \
	bseeffectfinetune.h \
	bseeffectnotevolume.h \
	bseeffectpatternbreak.h \
	bseeffectpatternjump.h \
	bseeffectvolumedelta.h \
	bseenums.h \
	bseexports.h \
	bsegconfig.h \
	bseglobals.h \
	bseglue.h \
	bseinstrument.h \
	bseitem.h \
	bsemain.h \
	bsemath.h \
	bsemidiicontroller.h \
	bsemididevice.h \
	bsemididevice-null.h \
	bsemididevice-oss.h \
	bsemidievent.h \
	bsemidimodule.h \
	bsemidinotifier.h \
	bsemidireceiver.h \
	bsemidisynth.h \
	bsemidisynthinput.h \
	bseobject.h \
	bsepart.h \
	bsepcminput.h \
	bsepcmoutput.h \
	bseparam.h \
	bseparasite.h \
	bsepattern.h \
	bsepatterngroup.h \
	bsepcmdevice.h \
	bsepcmdevice-oss.h \
	bseplugin.h \
	bseprocedure.h \
	bseproject.h \
	bsescriptcontrol.h \
	bsescripthelper.h \
	bseserver.h \
	bsesnet.h \
	bsesnooper.h \
	bsesong.h \
	bsesongthread.h \
	bsesource.h \
	bsestandardosc.h \
	bsestandardsynths.h \
	bsestorage.h \
	bsesubinstrument.h \
	bsesubiport.h \
	bsesubkeyboard.h \
	bsesuboport.h \
	bsesubsynth.h \
	bsesuper.h \
	bsetype.h \
	bseutils.h \
	bsewave.h \
	bsewaveosc.h \
	bsewaverepo.h \
	bswcommon.h \
	bswprivate.h \
	glib-extra.h \
	$(TOYPROF_H_SOURCES) \
@STRIP_END@
# BSE header files that don't get installed
bse_private_h_sources = @STRIP_BEGIN@ \
@STRIP_END@
# BSE C sources to build the library from
bse_c_sources = @STRIP_BEGIN@ \
	bseamplifier.c \
	bsebindata.c \
	bsebiquadfilter.c \
	bsecategories.c \
	bsecomwire.c \
	bseconstant.c \
	bsecontainer.c \
	bsedatapocket.c \
	bseeditablesample.c \
	bseeffect.c \
	bseeffectbalance.c \
	bseeffectfinetune.c \
	bseeffectnotevolume.c \
	bseeffectpatternbreak.c \
	bseeffectpatternjump.c \
	bseeffectvolumedelta.c \
	bseenums.c \
	bsegconfig.c \
	bseglobals.c \
	bseglue.c \
	bseinstrument.c \
	bseitem.c \
	bsemain.c \
	bsemath.c \
	bsemidiicontroller.c \
	bsemididevice.c \
	bsemididevice-null.c \
	bsemididevice-oss.c \
	bsemidievent.c \
	bsemidimodule.c \
	bsemidinotifier.c \
	bsemidireceiver.c \
	bsemidisynth.c \
	bsemidisynthinput.c \
	bseobject.c \
	bsepart.c \
	bsepcminput.c \
	bsepcmoutput.c \
	bseparam.c \
	bseparasite.c \
	bsepattern.c \
	bsepatterngroup.c \
	bsepcmdevice.c \
	bsepcmdevice-oss.c \
	bseplugin.c \
	bseprocedure.c \
	bseproject.c \
	bsescriptcontrol.c \
	bsescripthelper.c \
	bseserver.c \
	bsesnet.c \
	bsesnooper.c \
	bsesong.c \
	bsesongthread.c \
	bsesource.c \
	bsestandardosc.c \
	bsestandardsynths.c \
	bsestorage.c \
	bsesubinstrument.c \
	bsesubiport.c \
	bsesubkeyboard.c \
	bsesuboport.c \
	bsesubsynth.c \
	bsesuper.c \
	bsetype.c \
	bseutils.c \
	bsewave.c \
	bsewaveosc.c \
	bsewaverepo.c \
	bswcommon.c \
	glib-extra.c \
	$(GSL_C_SRC) \
	$(TOYPROF_C_SOURCES) \
@STRIP_END@
# procedure sources
bse_proc_sources = @STRIP_BEGIN@ \
	bsecontainer.proc \
	bsedatapocket.proc \
	bseeditablesample.proc \
	bseenums.proc \
	bsehelloworld.proc \
	bsemidinotifier.proc \
	bseobject.proc \
	bsepart.proc \
	bseprocedure.proc \
	bseproject.proc \
	bsescriptcontrol.proc \
	bsescripthelper.proc \
	bseserver.proc \
	bsesong.proc \
	bsesource.proc \
	bsesnet.proc \
	bseitem.proc \
	bsewave.proc \
	bsewaveosc.proc \
	bsewaverepo.proc \
@STRIP_END@
bse_proc_gen_sources = @STRIP_BEGIN@ \
	bsecontainer.gen-proc.c \
	bsedatapocket.gen-proc.c \
	bseenums.gen-proc.c \
	bseeditablesample.gen-proc.c \
	bsehelloworld.gen-proc.c \
	bsemidinotifier.gen-proc.c \
	bseobject.gen-proc.c \
	bsepart.gen-proc.c \
	bseprocedure.gen-proc.c \
	bseproject.gen-proc.c \
	bsescriptcontrol.gen-proc.c \
	bsescripthelper.gen-proc.c \
	bseserver.gen-proc.c \
	bsesong.gen-proc.c \
	bsesource.gen-proc.c \
	bsesnet.gen-proc.c \
	bseitem.gen-proc.c \
	bsewave.gen-proc.c \
	bsewaveosc.gen-proc.c \
	bsewaverepo.gen-proc.c \
@STRIP_END@
# c sources for builtin plugins
bse_builtin_c_sources = @STRIP_BEGIN@ \
	bsepattern.proc.c \
	$(bse_proc_gen_sources) \
@STRIP_END@
# non-compile and non-install sources required
bse_extra_sources = @STRIP_BEGIN@ \
	mkbuiltinlist.pl \
	mktypes.pl \
	mkcalls.pl \
	bseamplifier-aux.c \
	bseconfig.h.in \
	bsepcmmodule.c \
	bsemidimodule.c \
	bsemarshal.list \
@STRIP_END@
# we use our own built_sources variable rules instead of BUILT_SOURCES,
# since handles BUILT_SOURCES strange and wrong on some occasions (mostly
# wrt dependancies)
# we generate frequently rebuild files piggyback on a stamp file, so sources
# depending on them only get rebuild when the built source actually changed
# content
#
# h-sources to build and install
bse_built_h_sources = @STRIP_BEGIN@ \
	bsegentypes.h \
	bsemarshal.h \
@STRIP_END@
# c-sources to build and compile
bse_built_c_sources = @STRIP_BEGIN@ \
	bseenum_arrays.c \
	bseenum_list.c \
	bsebuiltin_externs.c \
	bsebuiltin_array.c \
	bsegentypes.c \
	bsegentype_array.c \
	bsemarshal.h \
	bsemarshal.c \
@STRIP_END@
# built-deps
bseenums.c: bseenum_arrays.c bseenum_list.c
bseobject.c: bsemarshal.c
bseplugin.c: bsebuiltin_externs.c bsebuiltin_array.c
bsetype.c: bsegentypes.c bsegentype_array.c

#
# setup BSE sources and their dependancies
#
bse_h_sources = $(bse_private_h_sources) $(bse_public_h_sources) $(bse_built_h_sources)
libbseinclude_HEADERS = $(bse_public_h_sources) $(bse_built_h_sources)
libbse_la_SOURCES = $(bse_c_sources) $(bse_builtin_c_sources)
MAINTAINERCLEANFILES += $(bse_built_h_sources) $(bse_built_c_sources)
EXTRA_HEADERS +=
EXTRA_DIST += $(bse_private_h_sources) $(bse_proc_sources)
EXTRA_DIST += $(bse_built_c_sources) $(bse_built_h_sources) $(bse_extra_sources)
GLIB_MKENUMS = glib-mkenums

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-sbgth xgen-bgtc xgen-bgtac xgen-sbeac xgen-belc xgen-bbec xgen-bbac xgen-bnac xgen-bmx xgen-bmh xgen-bmc
CLEANFILES += $(gen_sources) @MISSING_PERL5@ $(bse_proc_gen_sources)
$(OBJECTS): $(bse_built_h_sources)
# piggyback generation through sideeffects needs NOP-rules for dep-tracking
bsegentypes.h: stamp-bsegentypes.h ;
# normal autogeneration rules
stamp-bsegentypes.h: @MISSING_PERL5@ $(srcdir)/mktypes.pl
stamp-bsegentypes.h: @MISSING_PERL5@ @BIRNET_N@ $(bse_c_sources)
stamp-bsegentypes.h: @MISSING_PERL5@ $(bse_public_h_sources)
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "#define BSE_TYPE_@ENUMSHORT@\t    (BSE_TYPE_ID (@EnumName@))\n" \
	  --eprod "extern GType BSE_TYPE_ID (@EnumName@);" \
	    $(bse_public_h_sources) > xgen-sbgth \
	&& $(PERL) $(srcdir)/mktypes.pl --externs $(bse_c_sources) >> xgen-sbgth \
	&& (cmp -s xgen-sbgth bsegentypes.h || cp xgen-sbgth bsegentypes.h) \
	&& rm -f xgen-sbgth \
	&& echo timestamp > $@
bsegentypes.c: @MISSING_PERL5@ bsegentypes.h $(srcdir)/mktypes.pl
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --eprod "\nGType BSE_TYPE_ID (@EnumName@) = 0;" \
	    $(bse_public_h_sources) > xgen-bgtc \
	&& $(PERL) $(srcdir)/mktypes.pl --interns --export-proto $(bse_c_sources) >> xgen-bgtc \
	&& cp xgen-bgtc $@ \
	&& rm -f xgen-bgtc
bsegentype_array.c: @MISSING_PERL5@ bsegentypes.c
	cd $(srcdir) \
	&& $(PERL) $(srcdir)/mktypes.pl --array $(bse_c_sources) > xgen-bgtac \
	&& cp xgen-bgtac $@ \
	&& rm -f xgen-bgtac
bseenum_arrays.c: @MISSING_PERL5@ $(bse_public_h_sources)
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	  --vhead "/* @EnumName@\n */\n" \
	  --vhead "static G@Type@Value @enum_name@_values[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "  { 0, NULL, NULL }\n};\n" \
	    $(bse_public_h_sources) > xgen-sbeac \
	&& cp xgen-sbeac $@ \
	&& rm -f xgen-sbeac
bseenum_list.c: @MISSING_PERL5@ bseenum_arrays.c
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &BSE_TYPE_ID (@EnumName@), @enum_name@_values }," \
	    $(bse_public_h_sources) > xgen-belc \
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
$(srcdir)/bsemarshal.h: @MISSING_PERL5@ bsemarshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=bse_marshal bsemarshal.list --header >> xgen-bmh \
	&& cp xgen-bmh bsemarshal.h \
	&& rm -f xgen-bmh
$(srcdir)/bsemarshal.c: @MISSING_PERL5@ bsemarshal.h
	cd $(srcdir) \
	&& glib-genmarshal --prefix=bse_marshal bsemarshal.list --body >> xgen-bmc \
	&& cp xgen-bmc bsemarshal.c \
	&& rm -f xgen-bmc
%.gen-proc.c: %.proc $(srcdir)/mkcproc.pl
	$(srcdir)/mkcproc.pl $< >$@


#
# convenience targets for the builtsources
#
.PHONY: builtsources clean-builtsources
clean-builtsources:
	rm -f $(bse_built_c_sources) $(bse_built_h_sources) stamp-*
builtsources: $(bse_built_h_sources) $(bse_built_c_sources)


#
# private (birnet) dependancy to make TAGS file after each build
#
all: @MISSING_ETAGS@ @BIRNET_Y@ # TAGS


# TOYPROF: poor man's profiling toy
TOYPROF_H_SOURCES = toyprof.h
TOYPROF_C_SOURCES = toyprof.c
EXTRA_DIST += toyprof.pl toyprof.README


# --- GSL targets ---
GSL_progs_ldadd = $(progs_LDADD)
GSL_cc_dummy    =
gslincludedir   = $(includedir)/gsl
include gslmakefile.inc


#
# other programs, we want to compile
#
noinst_PROGRAMS = bsequery bsemagic bseinfo $(GSL_NOINST_PROGS)
# source files
bsequery_SOURCES = bsequery.c
bsemagic_SOURCES = bsemagictest.c
bseinfo_SOURCES = bseinfo.c
# link programs against BSE
progs_LDADD = libbse.la
bsequery_LDADD = $(progs_LDADD)
bsemagic_LDADD = $(progs_LDADD)
bseinfo_LDADD = $(progs_LDADD)


#
# auxillary files
#
EXTRA_DIST += \
	raw2bse \
	TODO
