# BSE - Better Sound Engine
include $(top_srcdir)/Makefile.decl

SUBDIRS = icons zintern . tests

# need -I$(top_srcdir) for <bse/bsedefs.hh>
# need -I$(top_builddir) for <sfi/sficonfig.h>
# need -I$(srcdir) for "bseserver.hh" in .genprc.cc
# need -I. (builddir) for "bsebasics.genidl.hh" in bsebasics.cc
INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS     += @DEFINE__FILE_DIR__@ $(strip \
	$(patsubst %, -DG_LOG_DOMAIN=\"BSE\" -DBSE_COMPILATION, \
	              $(filter $(<F), $(bse_sources) $(bse_sources))) \
)
AM_CXXFLAGS  = $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE -DG_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS
AIDACC_VDEBUG = $(AIDACC) $(if $(findstring 1, $(V)), --aida-debug)

#
# setup source file variables
#
# BSE public header files
bse_public_headers = $(strip \
	gsldefs.hh \
	gslfft.hh		gsloscillator.hh	gsldatahandle.hh	gslwavechunk.hh \
	gslfilter.hh		gslcommon.hh \
	gsldatahandle-vorbis.hh	gslvorbis-enc.hh	gsldatacache.hh	gslvorbis-cutter.hh \
	gsldatahandle-mad.hh	           	gslfilehash.hh	gsldatautils.hh	\
	gslwaveosc.hh		gslosctable.hh	gslmagic.hh	\
	\
	bsegentypes.h \
	\
	bse.hh	bsedefs.hh	bseexports.hh	bseconfig.h	bsegenclosures.hh \
	bseincluder.hh	ladspa.hh	bseenginenode.hh bseieee754.hh \
	bsecore.hh		\
	bseengine.hh		bseenginemaster.hh	bseengineschedule.hh		bseengineutils.hh \
	bsebus.hh		bsecategories.hh 	\
	bsefilter.hh    		bsebiquadfilter.hh	\
	bseconstant.hh		bseconstvalues.hh	bsecontainer.hh			bsecontextmerger.hh \
	bsedatapocket.hh		bseeditablesample.hh	bseenums.hh			bsegconfig.hh \
	bseglobals.hh		bseglue.hh		bseitem.hh			bsejanitor.hh \
	bsemain.hh		bsemath.hh		bsemathsignal.hh			bseladspa.hh \
	bsemidicontroller.hh	bsemididevice.hh		bsedevice.hh 			\
	bsemididevice-null.hh	bsemididevice-oss.hh	bsemidievent.hh			bsemidinotifier.hh \
	bsemidireceiver.hh	bsemidisynth.hh		bseobject.hh			bsepart.hh \
	bsepcminput.hh		bsepcmoutput.hh		bseparam.hh			bseparasite.hh \
	bsepcmdevice.hh		bsepcmdevice-oss.hh	bsepcmdevice-null.hh		bseplugin.hh \
	bseprocedure.hh		bseproject.hh		bsescripthelper.hh		bseserver.hh \
	bsesnet.hh		bsesnooper.hh		bsesong.hh			bsesequencer.hh \
	bsesource.hh		bsestandardosc.hh	bsestandardsynths.hh		bsestorage.hh \
	bseinstrumentoutput.hh	bsesubiport.hh		bseinstrumentinput.hh		bsesuboport.hh \
	bsesubsynth.hh		bsesuper.hh		bsetrack.hh			bsetype.hh \
	bseutils.hh		bsemidivoice.hh		bsewave.hh			bsewaveosc.hh \
	bsecsynth.hh		bsewaverepo.hh		bseladspamodule.hh		bsepcmwriter.hh \
	bsecompat.hh		bseundostack.hh		bsemidiinput.hh			bsemididecoder.hh \
	bsenote.hh		bsemidifile.hh		bseblockutils.hh		\
	bsecxxvalue.hh		bsecxxutils.hh		bsecxxbase.hh			bsecxxclosure.hh \
	bsecxxarg.hh		bsecxxmodule.hh		bsecxxplugin.hh			bseloader.hh \
	bseresampler.hh		bseresamplerimpl.hh	\
)
# BSE C & C++ sources
bse_sources = $(strip \
	gslfft.cc		gsloscillator.cc	gsldatahandle.cc	gslwavechunk.cc \
	gslfilter.cc		gslcommon.cc \
	gsldatahandle-vorbis.cc	gslvorbis-enc.cc	gsldatacache.cc	gslvorbis-cutter.cc \
	gsldatahandle-mad.cc	           	gslfilehash.cc	gsldatautils.cc	\
	gslwaveosc.cc		gslosctable.cc	gslmagic.cc	                 \
	bsecore.cc		\
	bseengine.cc		bseenginemaster.cc	bseengineschedule.cc		bseengineutils.cc \
	bsebus.cc		bsecategories.cc 	\
	bsefilter.cc   		bsebiquadfilter.cc	bsefilter-ellf.cc	\
	bseconstant.cc		bseconstvalues.cc	bsecontainer.cc			bsecontextmerger.cc \
	bsedatapocket.cc	bseeditablesample.cc	bseenums.cc			bsegconfig.cc \
	bseglobals.cc		bseglue.cc		bseitem.cc			bsejanitor.cc \
	bsemain.cc		bsemath.cc		bsemathsignal.cc		bseladspa.cc \
	bsemidicontroller.cc	bsemididevice.cc 	bsedevice.cc 			\
	bsemididevice-null.cc	bsemididevice-oss.cc	bsemidievent.cc			bsemidinotifier.cc \
	bsemidireceiver.cc	bsemidisynth.cc		bseobject.cc			bsepart.cc \
	bsepcminput.cc		bsepcmoutput.cc		bseparam.cc			bseparasite.cc \
	bsepcmdevice.cc		bsepcmdevice-oss.cc	bsepcmdevice-null.cc		bseplugin.cc \
	bseprocedure.cc		bseproject.cc		bsescripthelper.cc		bseserver.cc \
	bsesnet.cc		bsesnooper.cc		bsesong.cc			bsesequencer.cc \
	bsesource.cc		bsestandardosc.cc	bsestandardsynths.cc		bsestorage.cc \
	bseinstrumentoutput.cc	bsesubiport.cc		bseinstrumentinput.cc		bsesuboport.cc \
	bsesubsynth.cc		bsesuper.cc		bsetrack.cc			bsetype.cc \
	bseutils.cc		bsemidivoice.cc		bsewave.cc			bsewaveosc.cc \
	bsecsynth.cc		bsewaverepo.cc		bseladspamodule.cc		bsepcmwriter.cc \
	bsecompat.cc		bseundostack.cc		bsemidiinput.cc			bsemididecoder.cc \
	bsenote.cc		bsemidifile.cc		bseblockutils.cc		\
	bsecxxvalue.cc		bsecxxutils.cc		bsecxxbase.cc			bsecxxclosure.cc \
	bsecxxarg.cc		bsecxxmodule.cc		bsecxxplugin.cc			bseloader.cc \
	bseresampler.cc 	bsedatahandle-resample.cc				bsedatahandle-fir.cc \
	bseloader-aiff.cc	bseloader-guspatch.cc	bseloader-oggvorbis.cc		bseloader-bsewave.cc \
	bseloader-mad.cc	bseloader-wav.cc	\
	bsebusmodule.cc		\
	bsebasics.cc		\
	bseprobe.cc		\
)
# BSE Synthesis Modules
bse_idl_sources =
bse_idl_sources += bsebusmodule.idl
$(srcdir)/bsebusmodule.cc: bsebusmodule.genidl.hh
bse_idl_sources += bsebasics.idl
$(srcdir)/bsecxxplugin.cc: bsebasics.genidl.hh
bse_idl_sources += bseprobe.idl
$(srcdir)/bseprobe.cc: bseprobe.genidl.hh
idl_built_sources = $(bse_idl_sources:.idl=.genidl.hh)
GENERATED += $(idl_built_sources)
$(srcdir)/bsebus.cc: bsebasics.genidl.hh
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
bse_proc_gen_sources = $(bse_proc_sources:.proc=.genprc.cc)
# non-compile and non-install sources required
EXTRA_DIST += $(strip \
	bsebasics.idl	bse.idl					\
	mktypes.pl	mkcalls.pl	mkcproc.pl		\
	bseconfig.h.in	bsepcmmodule.cc				\
	bsewave.header						\
	gsl-fftgen.pl	gsl-fftconf.sh	gsloscillator-aux.cc	\
	gslincluder.hh	gslwaveosc-aux.cc			\
)

# -pthread -> -lpthread hack (required for libtool library linkage)
pthread_2_lpthread = $(if $(findstring -pthread, $1), -lpthread)

#
# BSE library
#
lib_LTLIBRARIES              = libbse.la
libbseincludedir       	     = $(includedir)/bse
libbseinclude_HEADERS        = $(bse_public_headers) bse.idl $(bse_idl_sources) bsehack.idl $(idl_dummy_files)
nodist_libbseinclude_HEADERS = bsebasics.genidl.hh
libbse_la_SOURCES            = $(bse_sources) $(bse_proc_gen_sources)
libbse_la_LIBADD             = $(top_builddir)/birnet/libbirnet.o $(top_builddir)/sfi/libsfi.o $(BSE_LIBS) $(SFI_LIBS) $(RAPICORN_LIBS)
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
SFIDL_INC = --nostdinc -I$(top_builddir) -I$(top_srcdir) -I$(srcdir)


#
# rules to generate built sources
#
%.genprc.cc: @PERLRULE@ %.proc mkcproc.pl
	$(srcdir)/mkcproc.pl --funcname $@ --preprocess $< >$@
if WITH_PERLRULE
CLEANFILES += $(bse_proc_gen_sources)
else
MAINTAINERCLEANFILES += $(bse_proc_gen_sources)
endif
# gslfft.cc
GENERATED += gslfft.cc
$(srcdir)/gslfft.cc: @PERLRULE@ $(srcdir)/gsl-fftgen.pl $(srcdir)/gsl-fftconf.sh
	$(srcdir)/gsl-fftconf.sh '$(PERL) $(srcdir)/gsl-fftgen.pl' \"gslfft.hh\" >$@
# bsegentypes.h
GENERATED_EXTRA_DIST += bsegentypes.h
$(libbse_la_OBJECTS): bsegentypes.h
bsegentypes.h: @PERLRULE@ mktypes.pl $(filter-out bsegentypes.cc, $(bse_sources)) $(filter-out bsegentypes.h, $(bse_public_headers))
bsegentypes.h: @PERLRULE@ bsebasics.idl $(SFIDL)
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */" \
	    --eprod "#define BSE_TYPE_@ENUMSHORT@\t    (BSE_TYPE_ID (@EnumName@))\n" \
	    --eprod "extern GType BSE_TYPE_ID (@EnumName@);" \
	      $(filter-out bsegentypes.h, $(bse_public_headers)) \
	  && $(PERL) mktypes.pl --externs $(filter-out bsegentypes.cc, $(bse_sources)) \
	) > xgen-$(@F) \
	&& $(SFIDL) $(SFIDL_INC) --core-c --header $(srcdir)/bsebasics.idl >> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(srcdir)/bsegentypes.h || cp xgen-$(@F) $(srcdir)/bsegentypes.h) \
	&& rm -f xgen-$(@F)
$(bse_sources): bsegentypes.h
# bsegentypes.cc
GENERATED_EXTRA_DIST += bsegentypes.cc
$(libbse_la_OBJECTS): bsegentypes.cc
bsegentypes.cc: @PERLRULE@ bsegentypes.h mktypes.pl bsebasics.idl $(SFIDL)
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --eprod "\nGType BSE_TYPE_ID (@EnumName@) = 0;" \
	      $(bse_public_headers) \
	  && $(PERL) mktypes.pl --interns --export-proto $(bse_sources) \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# bseenum_arrays.cc
GENERATED_CLEANFILES += bseenum_arrays.cc
$(libbse_la_OBJECTS): bseenum_arrays.cc
bseenum_arrays.cc: bsegentypes.h
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
# bseenum_list.cc
GENERATED_CLEANFILES += bseenum_list.cc
$(libbse_la_OBJECTS): bseenum_list.cc
bseenum_list.cc: bsegentypes.h
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */" \
	    --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &BSE_TYPE_ID (@EnumName@), @enum_name@_values }," \
	      $(filter-out bsegentypes.h, $(bse_public_headers)) \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bsegentype_array.cc
GENERATED_EXTRA_DIST += bsegentype_array.cc
$(libbse_la_OBJECTS): bsegentype_array.cc
bsegentype_array.cc: @PERLRULE@ bsegentypes.h
	cd $(srcdir) \
	&& $(PERL) mktypes.pl --array $(bse_sources) > xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# bsebuiltin_externs.cc
GENERATED_EXTRA_DIST += bsebuiltin_externs.cc
#$(libbse_la_OBJECTS): bsebuiltin_externs.cc # forces complete rebuild when *.proc changes
bseplugin.cc: bsebuiltin_externs.cc
bsebuiltin_externs.cc: @PERLRULE@ $(bse_proc_gen_sources) mkcproc.pl
	cd $(srcdir) \
	&& $(PERL) mkcproc.pl --externs $(bse_proc_gen_sources) > xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# bsebuiltin_array.cc
GENERATED_EXTRA_DIST += bsebuiltin_array.cc
#$(libbse_la_OBJECTS): bsebuiltin_array.cc # forces complete rebuild when *.proc changes
bseplugin.cc: bsebuiltin_array.cc
bsebuiltin_array.cc: @PERLRULE@ $(bse_proc_gen_sources) mkcproc.pl
	cd $(srcdir) \
	&& $(PERL) mkcproc.pl --functions $(bse_proc_gen_sources) > xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# bsegenbasics.cc
GENERATED_CLEANFILES += bsegenbasics.cc
$(srcdir)/bsebasics.cc: bsegenbasics.cc
bsegenbasics.cc: bsebasics.idl $(SFIDL) bsebasics.genidl.hh
	cd . \
	&& $(SFIDL) $(SFIDL_INC) --core-c --source --init sfidl_types_init $(srcdir)/bsebasics.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# setup generation of C++ objects from idl files
%.genidl.hh: $(srcdir)/%.idl $(SFIDL)
	$(SFIDL) --core-cxx --macro $(<F) $(SFIDL_INC) $< > $@ || (rm $@ ; exit 1 )


#
# this file describes the BSE public API and needs to be generated at the
# end of the build process
#
bsehack.idl: bsebasics.idl bseprocidl
	grep -v bsehack.idl $(srcdir)/bse.idl | $(SFIDL) $(SFIDL_INC) --list-types - > xgen-2-$(@F)
	echo "/* this file was generated from make $< */" > xgen-$(@F) \
	&& ./bseprocidl xgen-2-$(@F) >> xgen-$(@F) \
	&& cp xgen-$(@F) bsehack.idl \
	&& rm -f xgen-$(@F) xgen-2-$(@F)
CLEANFILES += bsehack.idl
noinst_DATA = bsehack.idl

# == BSE IDL API (AIDA) ==
AUXTYPES_PY = $(srcdir)/AuxTypes.py
bseserverapi.hh: $(srcdir)/bseapi.idl $(AUXTYPES_PY)
	$(AM_V_GEN)
	$(Q) ${AIDACC} -x CxxStub -x $(AUXTYPES_PY) -G serverhh -G iface-postfix=Iface \
	  -G cppguard=__BST_BSESERVERAPI_HH_ $< -o xgen-$(@F)
	$(Q) mv xgen-$(@F) $@
GENERATED += bseserverapi.hh
bseserverapi.cc: $(srcdir)/bseapi.idl $(AUXTYPES_PY)
	$(AM_V_GEN)
	$(Q) ${AIDACC} -x CxxStub -x $(AUXTYPES_PY) -G servercc -G iface-postfix=Iface \
	  -G cppguard=__BST_BSESERVERAPI_CC_ $< -o xgen-$(@F)
	$(Q) mv xgen-$(@F) $@
GENERATED += bseserverapi.cc

# == GENERATED targets ==
generated: clean-generated $(GENERATED)
	$(Q) $(MAKE) $(AM_MAKEFLAGS) clean-generated
	$(Q) $(MAKE) generated-files
generated-files: $(GENERATED)
clean-generated:
	rm -f $(GENERATED) stamp-*
clean-local: clean-generated
.PHONY: clean-generated generated-files generated
bseutils.lo: $(GENERATED)	# need dependency to force file generation

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
bsequery_SOURCES = bsequery.cc cxxdummy.cc
bsequery_LDADD   = $(progs_LDADD)

noinst_PROGRAMS   += bseautodoc
bseautodoc_SOURCES = bseautodoc.cc
bseautodoc_LDADD   = $(progs_LDADD)

noinst_PROGRAMS += bseinfo
bseinfo_SOURCES  = bseinfo.cc
bseinfo_LDADD    = $(progs_LDADD)

# == Tests ==
check-oldidl: oldidl.idl AuxTypes.py
	@echo "  CHECK " $<
	$(Q) $(AIDACC_VDEBUG) -x $(srcdir)/AuxTypes.py $<
.PHONY: check-oldidl
check-local: check-oldidl
