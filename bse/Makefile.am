# BSE - Better Sound Engine
# Copyright (C) 1998-2002 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = icons zintern . tests

# need -I$(top_srcdir) for <bse/bsedefs.h>
# need -I$(top_builddir) for <sfi/sficonfig.h>
# need -I$(srcdir) for "bseserver.h" in .genprc.c
# need -I. (builddir) for "bsecore.genidl.hh" in bsecore.cc
INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(BSE_CFLAGS) -DG_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS
DEFS     += $(strip \
	$(patsubst %, -DG_LOG_DOMAIN=\"BSE\" -DBSE_COMPILATION, \
	              $(filter $(<F), $(bse_sources) $(bse_sources))) \
)

#
# setup source file variables
#
# BSE public header files
bse_public_headers = $(strip \
	gsldefs.h \
	gslfft.h		gsloscillator.h	gsldatahandle.h	gslwavechunk.h \
	gslfilter.h		gslcommon.h \
	gsldatahandle-vorbis.h	gslvorbis-enc.h	gsldatacache.h	gslvorbis-cutter.h \
	gsldatahandle-mad.h	           	gslfilehash.h	gsldatautils.h	\
	gslwaveosc.h		gslosctable.h	gslmagic.h	\
	\
	bsegentypes.h \
	\
	bse.h	bsedefs.h	bseexports.h	bseconfig.h	bsegenclosures.h \
	bseincluder.h	ladspa.h	bseenginenode.h bseieee754.h \
	\
	bseengine.h		bseenginemaster.h	bseengineschedule.h		bseengineutils.h \
	bsebus.h		bsecategories.h 	\
	bsefilter.h    		bsebiquadfilter.h	\
	bseconstant.h		bseconstvalues.h	bsecontainer.h			bsecontextmerger.h \
	bsedatapocket.h		bseeditablesample.h	bseenums.h			bsegconfig.h \
	bseglobals.h		bseglue.h		bseitem.h			bsejanitor.h \
	bsemain.h		bsemath.h		bsemathsignal.h			bseladspa.h \
	bsemidicontroller.h	bsemididevice.h		bsedevice.h 			\
	bsemididevice-null.h	bsemididevice-oss.h	bsemidievent.h			bsemidinotifier.h \
	bsemidireceiver.h	bsemidisynth.h		bseobject.h			bsepart.h \
	bsepcminput.h		bsepcmoutput.h		bseparam.h			bseparasite.h \
	bsepcmdevice.h		bsepcmdevice-oss.h	bsepcmdevice-null.h		bseplugin.h \
	bseprocedure.h		bseproject.h		bsescripthelper.h		bseserver.h \
	bsesnet.h		bsesnooper.h		bsesong.h			bsesequencer.h \
	bsesource.h		bsestandardosc.h	bsestandardsynths.h		bsestorage.h \
	bseinstrumentoutput.h	bsesubiport.h		bseinstrumentinput.h		bsesuboport.h \
	bsesubsynth.h		bsesuper.h		bsetrack.h			bsetype.h \
	bseutils.h		bsemidivoice.h		bsewave.h			bsewaveosc.h \
	bsecsynth.h		bsewaverepo.h		bseladspamodule.h		bsepcmwriter.h \
	bsecompat.h		bseundostack.h		bsemidiinput.h			bsemididecoder.h \
	bsenote.h		bsemidifile.h		bseblockutils.hh		\
	bsecxxvalue.hh		bsecxxutils.hh		bsecxxbase.hh			bsecxxclosure.hh \
	bsecxxarg.hh		bsecxxmodule.hh		bsecxxplugin.hh			bseloader.h \
	bseresampler.hh		bseresamplerimpl.hh	\
)
# BSE C & C++ sources
bse_sources = $(strip \
	gslfft.c		gsloscillator.c	gsldatahandle.c	gslwavechunk.c \
	gslfilter.c		gslcommon.c \
	gsldatahandle-vorbis.c	gslvorbis-enc.c	gsldatacache.c	gslvorbis-cutter.c \
	gsldatahandle-mad.c	           	gslfilehash.c	gsldatautils.c	\
	gslwaveosc.c		gslosctable.c	gslmagic.c	                 \
	\
	bseengine.c		bseenginemaster.c	bseengineschedule.c		bseengineutils.c \
	bsebus.cc		bsecategories.cc 	\
	bsefilter.cc   		bsebiquadfilter.cc	bsefilter-ellf.cc	\
	bseconstant.cc		bseconstvalues.cc	bsecontainer.cc			bsecontextmerger.cc \
	bsedatapocket.cc	bseeditablesample.cc	bseenums.cc			bsegconfig.cc \
	bseglobals.cc		bseglue.cc		bseitem.c			bsejanitor.c \
	bsemain.cc		bsemath.c		bsemathsignal.c			bseladspa.cc \
	bsemidicontroller.c	bsemididevice.c 	bsedevice.cc 			\
	bsemididevice-null.c	bsemididevice-oss.c	bsemidievent.c			bsemidinotifier.c \
	bsemidireceiver.cc	bsemidisynth.c		bseobject.c			bsepart.c \
	bsepcminput.c		bsepcmoutput.c		bseparam.c			bseparasite.c \
	bsepcmdevice.c		bsepcmdevice-oss.c	bsepcmdevice-null.c		bseplugin.c \
	bseprocedure.c		bseproject.c		bsescripthelper.c		bseserver.c \
	bsesnet.c		bsesnooper.c		bsesong.c			bsesequencer.cc \
	bsesource.c		bsestandardosc.c	bsestandardsynths.c		bsestorage.c \
	bseinstrumentoutput.cc	bsesubiport.c		bseinstrumentinput.cc		bsesuboport.c \
	bsesubsynth.c		bsesuper.c		bsetrack.c			bsetype.c \
	bseutils.c		bsemidivoice.c		bsewave.c			bsewaveosc.c \
	bsecsynth.cc		bsewaverepo.c		bseladspamodule.c		bsepcmwriter.c \
	bsecompat.cc		bseundostack.c		bsemidiinput.c			bsemididecoder.c \
	bsenote.cc		bsemidifile.c		bseblockutils.cc		\
	bsecxxvalue.cc		bsecxxutils.cc		bsecxxbase.cc			bsecxxclosure.cc \
	bsecxxarg.cc		bsecxxmodule.cc		bsecxxplugin.cc			bseloader.c \
	bseresampler.cc 	bsedatahandle-resample.cc				bsedatahandle-fir.cc \
	bseloader-aiff.c	bseloader-guspatch.cc	bseloader-oggvorbis.c		bseloader-bsewave.c \
	bseloader-mad.c		bseloader-wav.c		\
	bsebusmodule.cc		\
	bsecore.cc		\
	bseprobe.cc		\
)
# BSE Synthesis Modules
bse_idl_sources =
bse_idl_sources += bsebusmodule.idl
$(srcdir)/bsebusmodule.cc: bsebusmodule.genidl.hh
bse_idl_sources += bsecore.idl
$(srcdir)/bsecxxplugin.cc: bsecore.genidl.hh
bse_idl_sources += bseprobe.idl
$(srcdir)/bseprobe.cc: bseprobe.genidl.hh
idl_built_sources = $(bse_idl_sources:.idl=.genidl.hh)
# idl files not used for build rules
idl_dummy_files = $(strip	\
	bsecxxmodule.idl	\
	bsecxxbase.idl		\
)
# BSE procedure sources
bse_proc_sources = $(strip \
	bsecategories.proc	bsecontainer.proc	bsedatapocket.proc	bseeditablesample.proc	bseenums.proc	bsemidinotifier.proc	\
	bsejanitor.proc		bsepart.proc		bseparasite.proc	bseprocedure.proc	bseproject.proc	bsescripthelper.proc	\
	bseserver.proc		bsesong.proc		bsebus.proc		bsesource.proc		bsecsynth.proc	bsesnet.proc		\
	bsetrack.proc		bseitem.proc		bsewave.proc		bsewaveosc.proc		bsewaverepo.proc			\
)
bse_proc_gen_sources = $(bse_proc_sources:.proc=.genprc.c)
# non-compile and non-install sources required
EXTRA_DIST += $(strip \
	bsecore.idl	bse.idl					\
	mktypes.pl	mkcalls.pl	mkcproc.pl		\
	bseconfig.h.in	bsepcmmodule.c				\
	bsewave.header						\
	gsl-fftgen.pl	gsl-fftconf.sh	gsloscillator-aux.c	\
	gslincluder.c	gslwaveosc-aux.c			\
)

# -pthread -> -lpthread hack (required for libtool library linkage)
pthread_2_lpthread = $(if $(findstring -pthread, $1), -lpthread)

#
# BSE library
#
lib_LTLIBRARIES              = libbse.la
libbseincludedir       	     = $(includedir)/bse
libbseinclude_HEADERS        = $(bse_public_headers) bse.idl $(bse_idl_sources) bsehack.idl $(idl_dummy_files)
nodist_libbseinclude_HEADERS = bsecore.genidl.hh
libbse_la_SOURCES            = $(bse_sources) $(bse_proc_gen_sources)
libbse_la_LIBADD             = $(top_builddir)/birnet/libbirnet.o $(top_builddir)/sfi/libsfi.o $(BSE_LIBS) $(SFI_LIBS)
libbse_la_LDFLAGS            = $(strip				\
	-Wl,--version-script=$(srcdir)/ldscript.map		\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-release $(LT_RELEASE) 					\
	-no-undefined 						\
	$(call pthread_2_lpthread, $(BSE_LIBS) $(SFI_LIBS))	\
)
EXTRA_DIST += ldscript.map
#	-Wl,-Bsymbolic
# setup sources, their dependancies and commands
CLEANFILES += $(idl_built_sources)
EXTRA_HEADERS +=
EXTRA_DIST += $(bse_proc_sources) engine-mplan.txt
GLIB_MKENUMS = glib-mkenums
# WATCH OUT: $(SFIDL) works only builddir relative
SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)


#
# rules to generate built sources
#
%.genprc.c: @PERLRULE@ %.proc mkcproc.pl
	$(srcdir)/mkcproc.pl --funcname $@ --preprocess $< >$@
if WITH_PERLRULE
CLEANFILES += $(bse_proc_gen_sources)
else
MAINTAINERCLEANFILES += $(bse_proc_gen_sources)
endif
# gslfft.c
GENERATED += gslfft.c
$(srcdir)/gslfft.c: @PERLRULE@ $(srcdir)/gsl-fftgen.pl $(srcdir)/gsl-fftconf.sh
	$(srcdir)/gsl-fftconf.sh '$(PERL) $(srcdir)/gsl-fftgen.pl' \"gslfft.h\" >$@
# bsegentypes.h
GENERATED_EXTRA_DIST += bsegentypes.h
$(libbse_la_OBJECTS): bsegentypes.h
bsegentypes.h: @PERLRULE@ mktypes.pl $(filter-out bsegentypes.c, $(bse_sources)) $(filter-out bsegentypes.h, $(bse_public_headers))
bsegentypes.h: @PERLRULE@ bsecore.idl $(SFIDL)
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */" \
	    --eprod "#define BSE_TYPE_@ENUMSHORT@\t    (BSE_TYPE_ID (@EnumName@))\n" \
	    --eprod "extern GType BSE_TYPE_ID (@EnumName@);" \
	      $(filter-out bsegentypes.h, $(bse_public_headers)) \
	  && $(PERL) mktypes.pl --externs $(filter-out bsegentypes.c, $(bse_sources)) \
	) > xgen-$(@F) \
	&& $(SFIDL) $(SFIDL_INC) --core-c --header $(srcdir)/bsecore.idl >> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(srcdir)/bsegentypes.h || cp xgen-$(@F) $(srcdir)/bsegentypes.h) \
	&& rm -f xgen-$(@F)
$(bse_sources): bsegentypes.h
# bsegentypes.c
GENERATED_EXTRA_DIST += bsegentypes.c
$(libbse_la_OBJECTS): bsegentypes.c
bsegentypes.c: @PERLRULE@ bsegentypes.h mktypes.pl bsecore.idl $(SFIDL)
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --eprod "\nGType BSE_TYPE_ID (@EnumName@) = 0;" \
	      $(bse_public_headers) \
	  && $(PERL) mktypes.pl --interns --export-proto $(bse_sources) \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# bseenum_arrays.c
GENERATED_CLEANFILES += bseenum_arrays.c
$(libbse_la_OBJECTS): bseenum_arrays.c
bseenum_arrays.c: bsegentypes.h
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	    --vhead "/* @EnumName@\n */\n" \
	    --vhead "static G@Type@Value @enum_name@_values[] = {" \
	    --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	    --vtail "  { 0, NULL, NULL }\n};\n" \
	      $(filter-out bsegentypes.h, $(bse_public_headers)) \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bseenum_list.c
GENERATED_CLEANFILES += bseenum_list.c
$(libbse_la_OBJECTS): bseenum_list.c
bseenum_list.c: bsegentypes.h
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */" \
	    --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &BSE_TYPE_ID (@EnumName@), @enum_name@_values }," \
	      $(filter-out bsegentypes.h, $(bse_public_headers)) \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bsegentype_array.c
GENERATED_EXTRA_DIST += bsegentype_array.c
$(libbse_la_OBJECTS): bsegentype_array.c
bsegentype_array.c: @PERLRULE@ bsegentypes.h
	cd $(srcdir) \
	&& $(PERL) mktypes.pl --array $(bse_sources) > xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# bsebuiltin_externs.c
GENERATED_EXTRA_DIST += bsebuiltin_externs.c
#$(libbse_la_OBJECTS): bsebuiltin_externs.c # forces complete rebuild when *.proc changes
bseplugin.c: bsebuiltin_externs.c
bsebuiltin_externs.c: @PERLRULE@ $(bse_proc_gen_sources) mkcproc.pl
	cd $(srcdir) \
	&& $(PERL) mkcproc.pl --externs $(bse_proc_gen_sources) > xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# bsebuiltin_array.c
GENERATED_EXTRA_DIST += bsebuiltin_array.c
#$(libbse_la_OBJECTS): bsebuiltin_array.c # forces complete rebuild when *.proc changes
bseplugin.c: bsebuiltin_array.c
bsebuiltin_array.c: @PERLRULE@ $(bse_proc_gen_sources) mkcproc.pl
	cd $(srcdir) \
	&& $(PERL) mkcproc.pl --functions $(bse_proc_gen_sources) > xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# bsegencore.cc
GENERATED_CLEANFILES += bsegencore.cc
$(srcdir)/bsecore.cc: bsegencore.cc
bsegencore.cc: bsecore.idl $(SFIDL) bsecore.genidl.hh
	cd . \
	&& $(SFIDL) $(SFIDL_INC) --core-c --source --init sfidl_types_init $(srcdir)/bsecore.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# setup generation of C++ objects from idl files
%.genidl.hh: $(srcdir)/%.idl $(SFIDL)
	$(SFIDL) --core-cxx --macro $(<F) $(SFIDL_INC) $< > $@ || (rm $@ ; exit 1 )


#
# this file describes the BSE public API and needs to be generated at the
# end of the build process
#
bsehack.idl: bsecore.idl bseprocidl
	grep -v bsehack.idl $(srcdir)/bse.idl | $(SFIDL) $(SFIDL_INC) --list-types - > xgen-2-$(@F)
	echo "/* this file was generated from make $< */" > xgen-$(@F) \
	&& ./bseprocidl xgen-2-$(@F) >> xgen-$(@F) \
	&& cp xgen-$(@F) bsehack.idl \
	&& rm -f xgen-$(@F) xgen-2-$(@F)
CLEANFILES += bsehack.idl
noinst_DATA = bsehack.idl

#
# convenience targets for generated source files
#
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(GENERATED)
generated: clean-generated $(GENERATED) $(idl_built_sources)


#
# private (birnet) dependancy to make TAGS file after each build
#
all: # TAGS



#
# other programs, we want to compile
#
noinst_PROGRAMS = $(ALLTESTS)
progs_LDADD = libbse.la $(SFI_LIBS) -lm
# source files

noinst_PROGRAMS   += bseprocidl
bseprocidl_SOURCES = bseprocidl.cc
bseprocidl_LDADD   = $(progs_LDADD)

noinst_PROGRAMS += bsequery
bsequery_SOURCES = bsequery.c cxxdummy.cc
bsequery_LDADD   = $(progs_LDADD)

noinst_PROGRAMS   += bseautodoc
bseautodoc_SOURCES = bseautodoc.cc
bseautodoc_LDADD   = $(progs_LDADD)

noinst_PROGRAMS += bseinfo
bseinfo_SOURCES  = bseinfo.cc
bseinfo_LDADD    = $(progs_LDADD)

