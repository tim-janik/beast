# BSE - Bedevilled Sound Engine
# Copyright (C) 1998-2002 Tim Janik
#
## Makefile.am for BSE

SUBDIRS = icons zintern . tests

# need -I$(top_srcdir) for <bse/bsedefs.h>
# need -I$(top_builddir) for <sfi/sficonfig.h>
# need -I$(srcdir) for "bseserver.h" in .gen-proc.c
# need -I. (builddir) for "bseamplifier.gen-idl.h" in bseamplifier.cc
INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(BSE_CFLAGS) -DG_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS

#
# libraries to compile and install
#
lib_LTLIBRARIES = libbse.la

# due to automake-1.3 not featuring target specific defines,
# the G_LOG_DOMAIN needs to be special cased, depending on
# the source file sets ;(
DEFS += $(strip \
	$(patsubst %, -DG_LOG_DOMAIN=\"BSE\" -DBSE_COMPILATION, \
	              $(filter $(<F), $(bse_c_sources) $(bse_cc_sources))) \
)

# libtool stuff: set version and export symbols for resolving
libbseincludedir = $(includedir)/bse
libbse_la_LDFLAGS = $(strip \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic \
)
libbse_la_LIBADD = $(BSE_LIBS) -lm


#
# setup source file variables
#
# BSE header files for public installation (non-generated)
bse_public_h_sources = $(strip \
	bse.h	bsedefs.h	bseexports.h	bseconfig.h	bsegenclosures.h \
	bseincluder.h	ladspa.h	\
				            		bsebiquadfilter.h		bsecategories.h \
	bseconstant.h		bseconstvalues.h	bsecontainer.h			bsecontextmerger.h \
	bsedatapocket.h		bseeditablesample.h	bseenums.h			bsegconfig.h \
	bseglobals.h		bseglue.h		bseitem.h			bsejanitor.h \
	bsemain.h		bsemath.h		bsemidicontroller.h		bsemididevice.h \
	bsemididevice-null.h	bsemididevice-oss.h	bsemidievent.h			bsemidinotifier.h \
	bsemidireceiver.h	bsemidisynth.h		bseobject.h			bsepart.h \
	bsepcminput.h		bsepcmoutput.h		bseparam.h			bseparasite.h \
	bsepcmdevice.h		bsepcmdevice-oss.h	bseplugin.h			bseladspa.h \
	bseprocedure.h		bseproject.h		bsescripthelper.h		bseserver.h \
	bsesnet.h		bsesnooper.h		bsesong.h			bsessequencer.h \
	bsesource.h		bsestandardosc.h	bsestandardsynths.h		bsestorage.h \
	bseinstrumentoutput.h	bsesubiport.h		bseinstrumentinput.h		bsesuboport.h \
	bsesubsynth.h		bsesuper.h		bsetrack.h			bsetype.h \
	bseutils.h		bsemidivoice.h		bsewave.h			bsewaveosc.h \
	bsecsynth.h		bsewaverepo.h		bseladspamodule.h		bsepcmwriter.h \
	bsecompat.h		bseundostack.h		bsemidiinput.h			bsemididecoder.h \
	bsenote.h		\
	bsecxxvalue.h		bsecxxutils.h		bsecxxbase.h			bsecxxclosure.h \
	bsecxxarg.h		bsecxxmodule.h		bsecxxplugin.h			\
)
# BSE header files that don't get installed
bse_private_h_sources = $(strip \
)
# BSE C sources to build the library from
bse_c_sources = $(strip \
	$(GSL_C_SRC) \
				            		bsebiquadfilter.c		bsecategories.c \
	bseconstant.c		bseconstvalues.c	bsecontainer.c			bsecontextmerger.c \
	bsedatapocket.c		bseeditablesample.c	bseenums.c			bsegconfig.c \
	bseglobals.c		bseglue.c		bseitem.c			bsejanitor.c \
	bsemain.c		bsemath.c		bsemidicontroller.c		bsemididevice.c \
	bsemididevice-null.c	bsemididevice-oss.c	bsemidievent.c			bsemidinotifier.c \
	bsemidireceiver.cc	bsemidisynth.c		bseobject.c			bsepart.c \
	bsepcminput.c		bsepcmoutput.c		bseparam.c			bseparasite.c \
	bsepcmdevice.c		bsepcmdevice-oss.c	bseplugin.c			bseladspa.c \
	bseprocedure.c		bseproject.c		bsescripthelper.c		bseserver.c \
	bsesnet.c		bsesnooper.c		bsesong.c			bsessequencer.c \
	bsesource.c		bsestandardosc.c	bsestandardsynths.c		bsestorage.c \
	bseinstrumentoutput.c	bsesubiport.c		bseinstrumentinput.c		bsesuboport.c \
	bsesubsynth.c		bsesuper.c		bsetrack.c			bsetype.c \
	bseutils.c		bsemidivoice.c		bsewave.c			bsewaveosc.c \
	bsecsynth.c		bsewaverepo.c		bseladspamodule.c		bsepcmwriter.c \
	bsecompat.c		bseundostack.c		bsemidiinput.c			bsemididecoder.c \
	bsenote.c		\
	bsecxxvalue.cc		bsecxxutils.cc		bsecxxbase.cc			bsecxxclosure.cc \
	bsecxxarg.cc		bsecxxmodule.cc		bsecxxplugin.cc			\
)
# Synthesis Modules
bse_idl_sources = $(strip \
	bseamplifier.idl	\
)
$(srcdir)/bseamplifier.cc: bseamplifier.gen-idl.h
idl_built_sources += $(bse_idl_sources:.idl=.gen-idl.h)
bse_cc_sources = $(strip \
	bseamplifier.cc		\
)
# procedure sources
bse_proc_sources = $(strip \
	bsecategories.proc \
	bsecontainer.proc \
	bsedatapocket.proc \
	bseeditablesample.proc \
	bseenums.proc \
	bsemidinotifier.proc \
	bsejanitor.proc \
	bseobject.proc \
	bsepart.proc \
	bseprocedure.proc \
	bseproject.proc \
	bsescripthelper.proc \
	bseserver.proc \
	bsesong.proc \
	bsesource.proc \
	bsecsynth.proc \
	bsesnet.proc \
	bsetrack.proc \
	bseitem.proc \
	bsewave.proc \
	bsewaveosc.proc \
	bsewaverepo.proc \
)
bse_proc_gen_sources = $(strip \
	bsecategories.gen-proc.c \
	bsecontainer.gen-proc.c \
	bsedatapocket.gen-proc.c \
	bseenums.gen-proc.c \
	bseeditablesample.gen-proc.c \
	bsemidinotifier.gen-proc.c \
	bsejanitor.gen-proc.c \
	bseobject.gen-proc.c \
	bsepart.gen-proc.c \
	bseprocedure.gen-proc.c \
	bseproject.gen-proc.c \
	bsescripthelper.gen-proc.c \
	bseserver.gen-proc.c \
	bsesong.gen-proc.c \
	bsesource.gen-proc.c \
	bsecsynth.gen-proc.c \
	bsesnet.gen-proc.c \
	bsetrack.gen-proc.c \
	bseitem.gen-proc.c \
	bsewave.gen-proc.c \
	bsewaveosc.gen-proc.c \
	bsewaverepo.gen-proc.c \
)
# non-compile and non-install sources required
bse_extra_sources = $(strip \
	mktypes.pl \
	mkcalls.pl \
	mkcproc.pl \
	bseconfig.h.in \
	bsepcmmodule.c \
	bserecords.idl \
)
# we use our own built_sources variable rules instead of BUILT_SOURCES,
# since handles BUILT_SOURCES strange and wrong on some occasions (mostly
# wrt dependancies)
# we generate frequently rebuild files piggyback on a stamp file, so sources
# depending on them only get rebuild when the built source actually changed
# content
#
# h-sources to build and install
bse_built_h_sources = $(strip \
	bsegentypes.h \
)
# c-sources to build and compile
bse_built_c_sources = $(strip \
	bseenum_arrays.c \
	bseenum_list.c \
	bsebuiltin_externs.c \
	bsebuiltin_array.c \
	bsegentypes.c \
	bsegentype_array.c \
)
# built-deps
$(srcdir)/bseenums.c: bseenum_arrays.c bseenum_list.c
$(srcdir)/bseplugin.c: bsebuiltin_externs.c bsebuiltin_array.c
$(srcdir)/bsetype.c: bsegentypes.c bsegentype_array.c

#
# setup sources, their dependancies and commands
#
bse_h_sources = $(bse_private_h_sources) $(bse_public_h_sources) $(bse_built_h_sources)
libbseinclude_HEADERS = $(bse_public_h_sources) $(bse_built_h_sources) bse.idl $(bse_idl_sources)
libbse_la_SOURCES = $(bse_c_sources) $(bse_proc_gen_sources) $(bse_cc_sources)
MAINTAINERCLEANFILES += $(bse_built_h_sources) $(bse_built_c_sources)
CLEANFILES += $(idl_built_sources)
EXTRA_HEADERS +=
EXTRA_DIST += $(bse_private_h_sources) $(bse_proc_sources)
EXTRA_DIST += $(bse_built_c_sources) $(bse_built_h_sources) $(bse_extra_sources)
GLIB_MKENUMS = glib-mkenums
SFIDL = ../sfi/sfidl	# WATCH OUT: $(SFIDL) works only for srcdir==builddir with cd $(srcdir)


#
# rules to generate built sources
#
# setup autogeneration dependancies
tmp_files = xgen-sbgth xgen-bgtc xgen-bgtac xgen-sbeac xgen-belc xgen-bbec xgen-bbac \
	    xgen-bnac xgen-bmx xgen-bmh xgen-bmc xgen-bgasi
CLEANFILES += $(tmp_files) @DVLBUILD@ $(bse_proc_gen_sources)
$(OBJECTS): $(bse_built_h_sources)
# piggyback generation through sideeffects, needs NOP-rule for dep-tracking, must be in srcdir
$(srcdir)/bsegentypes.h: @DVLBUILD@ stamp-bsegentypes.h ;
# normal autogeneration rules
stamp-bsegentypes.h: @DVLBUILD@ mktypes.pl $(bse_c_sources) $(bse_public_h_sources)
stamp-bsegentypes.h: @DVLBUILD@ $(srcdir)/bserecords.idl $(SFIDL)
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "#define BSE_TYPE_@ENUMSHORT@\t    (BSE_TYPE_ID (@EnumName@))\n" \
	  --eprod "extern GType BSE_TYPE_ID (@EnumName@);" \
	    $(bse_public_h_sources) > xgen-sbgth \
	&& $(PERL) mktypes.pl --externs $(bse_c_sources) >> xgen-sbgth \
	&& $(SFIDL) --header --implementation --boxed $(srcdir)/bserecords.idl >> xgen-sbgth \
	&& (cmp -s xgen-sbgth bsegentypes.h || cp xgen-sbgth bsegentypes.h) \
	&& rm -f xgen-sbgth \
	&& echo timestamp > $(@F)
bsegentypes.c: @DVLBUILD@ bsegentypes.h mktypes.pl $(srcdir)/bserecords.idl $(SFIDL) # $(bse_c_sources)
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --eprod "\nGType BSE_TYPE_ID (@EnumName@) = 0;" \
	    $(bse_public_h_sources) >> xgen-bgtc \
	&& $(PERL) mktypes.pl --interns --export-proto $(bse_c_sources) >> xgen-bgtc \
	&& $(SFIDL) --source --implementation --boxed --init _sfidl_types_init bserecords.idl >> xgen-bgtc \
	&& cp xgen-bgtc $(@F) \
	&& rm -f xgen-bgtc
bsegentype_array.c: @DVLBUILD@ bsegentypes.c
	cd $(srcdir) \
	&& $(PERL) mktypes.pl --array $(bse_c_sources) > xgen-bgtac \
	&& cp xgen-bgtac $(@F) \
	&& rm -f xgen-bgtac
bseenum_arrays.c: @DVLBUILD@ $(bse_public_h_sources)
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	  --vhead "/* @EnumName@\n */\n" \
	  --vhead "static G@Type@Value @enum_name@_values[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "  { 0, NULL, NULL }\n};\n" \
	    $(bse_public_h_sources) > xgen-sbeac \
	&& cp xgen-sbeac $(@F) \
	&& rm -f xgen-sbeac
bseenum_list.c: @DVLBUILD@ bseenum_arrays.c
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &BSE_TYPE_ID (@EnumName@), @enum_name@_values }," \
	    $(bse_public_h_sources) > xgen-belc \
	&& cp xgen-belc $(@F) \
	&& rm -f xgen-belc
bsebuiltin_externs.c: @DVLBUILD@ $(bse_proc_gen_sources) mkcproc.pl
	cd $(srcdir) \
	&& $(PERL) mkcproc.pl --externs $(bse_proc_gen_sources) > xgen-bbec \
	&& cp xgen-bbec $(@F) \
	&& rm -f xgen-bbec
bsebuiltin_array.c: @DVLBUILD@ $(bse_proc_gen_sources) mkcproc.pl
	cd $(srcdir) \
	&& $(PERL) mkcproc.pl --functions $(bse_proc_gen_sources) > xgen-bbac \
	&& cp xgen-bbac $(@F) \
	&& rm -f xgen-bbac
%.gen-proc.c: %.proc mkcproc.pl
	$(srcdir)/mkcproc.pl --funcname $@ --preprocess $< >$@
# setup generation of C++ objects from idl files
BSEPGEN = ../sfi/bse-plugin-generator
%.gen-idl.h: $(srcdir)/%.idl $(BSEPGEN)
	$(BSEPGEN) --nostdinc -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )


#
# this file describes the BSE public API and needs to be generated at the
# end of the build process
#
bse.idl: $(srcdir)/bserecords.idl bseprocidl
	echo "/* this file was generated from make $< */" > xgen-bgasi \
	&& cat $(srcdir)/bserecords.idl >> xgen-bgasi \
	&& ./bseprocidl >> xgen-bgasi \
	&& (cmp -s xgen-bgasi bse.idl || cp xgen-bgasi bse.idl) \
	&& rm -f xgen-bgasi
CLEANFILES += bse.idl
noinst_DATA = bse.idl


#
# convenience targets for generated source files
#
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(bse_built_c_sources) $(bse_built_h_sources) stamp-*
generated: clean-generated $(bse_built_h_sources) $(bse_built_c_sources) $(idl_built_sources)


#
# private (birnet) dependancy to make TAGS file after each build
#
all: # TAGS


# --- GSL targets ---
GSL_progs_ldadd = $(progs_LDADD)
GSL_cc_dummy    =
gslincludedir   = $(includedir)/bse
include gslmakefile.inc


#
# other programs, we want to compile
#
noinst_PROGRAMS = bsequery bseautodoc bseinfo bseprocidl
# source files
bseprocidl_SOURCES = bseprocidl.cc
bsequery_SOURCES = bsequery.c cxxdummy.cc
bseautodoc_SOURCES = bseautodoc.c cxxdummy.cc
bseinfo_SOURCES = bseinfo.c cxxdummy.cc
# link programs against BSE
progs_LDADD = libbse.la $(top_builddir)/sfi/libsfi.la
bseprocidl_LDADD = $(progs_LDADD)
bsequery_LDADD = $(progs_LDADD)
bseautodoc_LDADD = $(progs_LDADD)
bseinfo_LDADD = $(progs_LDADD)

