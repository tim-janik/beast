# BEAST & BSE

include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)

SUBDIRS = icons . tests # pybse

DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN=\"BSE\" -DG_DISABLE_CONST_RETURNS -DBSE_COMPILATION
AM_CPPFLAGS += -I$(top_srcdir)
# need -I$(top_srcdir) for <bse/bsedefs.hh>
# need -I$(top_builddir) for <sfi/sficonfig.h>
# need -I. (builddir) for "bsebasics.genidl.hh" in bsebasics.cc
AIDACC_VDEBUG = $(AIDACC) $(if $(findstring 1, $(V)), --aida-debug)
AIDACC        = $(topdir)/aidacc/aidacc

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
	bse.hh	bsedefs.hh	bseexports.hh	bseconfig.h	bsegenclosures.hh \
	bseincluder.hh	ladspa.hh	bseenginenode.hh bseieee754.hh \
	bsestartup.hh		\
	bseengine.hh		bseenginemaster.hh	bseengineschedule.hh		bseengineutils.hh \
	bsebus.hh		bsecategories.hh 	\
	bsefilter.hh    		bsebiquadfilter.hh	\
	bseconstant.hh		bseconstvalues.hh	bsecontainer.hh			bsecontextmerger.hh \
							effectbase.hh			\
				bseeditablesample.hh	bseenums.hh			bsegconfig.hh \
	bseglobals.hh		bseglue.hh		bseitem.hh			\
	bsemain.hh		bsemath.hh		bsemathsignal.hh			bseladspa.hh \
	bsemidicontroller.hh	bsemididevice.hh		bsedevice.hh 			\
	bsemididevice-null.hh	bsemididevice-oss.hh	bsemidievent.hh			bsemidinotifier.hh \
	bsemidireceiver.hh	bsemidisynth.hh		bseobject.hh			bsepart.hh \
	bsepcminput.hh		bsepcmoutput.hh		bseparam.hh			\
	bsepcmdevice.hh		bsepcmdevice-oss.hh	bsepcmdevice-null.hh		bseplugin.hh \
	bseprocedure.hh		bseproject.hh						bseserver.hh \
	bsesnet.hh		bsesnooper.hh		bsesong.hh			bsesequencer.hh \
	bsesource.hh		bsestandardosc.hh	bsestandardsynths.hh		bsestorage.hh \
	bseinstrumentoutput.hh	bsesubiport.hh		bseinstrumentinput.hh		bsesuboport.hh \
        bsesoundfont.hh         bsesoundfontpreset.hh   bsesoundfontosc.hh              bsesoundfontrepo.hh \
	bsesubsynth.hh		bsesuper.hh		bsetrack.hh			bsetype.hh \
	bseutils.hh		bsemidivoice.hh		bsewave.hh			bsewaveosc.hh \
	bsecsynth.hh		bsewaverepo.hh		bseladspamodule.hh		bsepcmwriter.hh \
	bsecompat.hh		bseundostack.hh		bsemidiinput.hh			bsemididecoder.hh \
	bsenote.hh		bsemidifile.hh		bseblockutils.hh		\
	bsecxxvalue.hh		bsecxxutils.hh		bsecxxbase.hh			bsecxxclosure.hh \
	bsecxxarg.hh		bsecxxmodule.hh		bsecxxplugin.hh			bseloader.hh \
	bseresampler.hh		bseresamplerimpl.hh	bsedatahandle-flac.hh           \
	testobject.hh		\
)
# BSE C & C++ sources
bse_sources = $(strip \
				gsloscillator.cc	gsldatahandle.cc	gslwavechunk.cc \
	gslfilter.cc		gslcommon.cc \
	gsldatahandle-vorbis.cc	gslvorbis-enc.cc	gsldatacache.cc	gslvorbis-cutter.cc \
	gsldatahandle-mad.cc	           	gslfilehash.cc	gsldatautils.cc	\
	gslwaveosc.cc		gslosctable.cc	gslmagic.cc	                 \
	bsestartup.cc		\
	bseengine.cc		bseenginemaster.cc	bseengineschedule.cc		bseengineutils.cc \
	bsebus.cc		bsecategories.cc 	\
	bsefilter.cc   		bsebiquadfilter.cc	bsefilter-ellf.cc	\
	bseconstant.cc		bseconstvalues.cc	bsecontainer.cc			bsecontextmerger.cc \
				bseeditablesample.cc	bseenums.cc			bsegconfig.cc \
	bseglobals.cc		bseglue.cc		bseitem.cc			\
	bsemain.cc		bsemath.cc		bsemathsignal.cc		bseladspa.cc \
	bsemidicontroller.cc	bsemididevice.cc 	bsedevice.cc 			\
	bsemididevice-null.cc	bsemididevice-oss.cc	bsemidievent.cc			bsemidinotifier.cc \
	bsemidireceiver.cc	bsemidisynth.cc		bseobject.cc			bsepart.cc \
	bsepcminput.cc		bsepcmoutput.cc		bseparam.cc			\
	bsepcmdevice.cc		bsepcmdevice-oss.cc	bsepcmdevice-null.cc		bseplugin.cc \
	bseprocedure.cc		bseproject.cc						bseserver.cc \
	bsesnet.cc		bsesnooper.cc		bsesong.cc			bsesequencer.cc \
	bsesoundfontrepo.cc	bsesoundfont.cc		bsesoundfontpreset.cc		bsesoundfontosc.cc \
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
	bseloader-mad.cc	bseloader-wav.cc	bseloader-flac.cc               bsedatahandle-flac.cc \
	bsebusmodule.cc		\
	bsebasics.cc		\
	bseprobe.cc		\
	testobject.cc		\
)
# BSE Synthesis Modules
bse_idl_sources =
bse_idl_sources += bsebusmodule.idl
$(srcdir)/bsebusmodule.cc: bsebusmodule.genidl.hh
bse_idl_sources += bsebasics.idl
$(srcdir)/bsecxxplugin.cc: bsebasics.genidl.hh
idl_built_sources = $(bse_idl_sources:.idl=.genidl.hh)
GENERATED += $(idl_built_sources)
$(srcdir)/bsebus.cc: bsebasics.genidl.hh
# fix genidl implicit rule matching include file dependencies in parent directories
../bse/bsebasics.genidl.hh: ;
# idl files not used for build rules
idl_dummy_files = $(strip	\
	bsecxxmodule.idl	\
	bsecxxbase.idl		\
)

doc_sources = $(bse_idl_sources) $(bse_sources) $(bse_public_headers) bseapi.idl

# == doc_sources.lst ==
doc_sources.lst: Makefile
	$(AM_V_GEN)
	$(Q) echo $(patsubst %, bse/%, $(doc_sources)) > $@

#
# BSE library
#
bselib_LTLIBRARIES                   = libbse-@MAJOR@.la
libbse_@MAJOR@includedir             = $(bseincludedir)/bse
libbse_@MAJOR@include_HEADERS        = $(bse_public_headers) bse.idl $(bse_idl_sources) $(idl_dummy_files)
nodist_libbse_@MAJOR@include_HEADERS = bsebasics.genidl.hh bseapi_interfaces.hh bseapi_handles.hh bsehack.idl bsegentypes.h
libbse_@MAJOR@_la_SOURCES            = $(bse_sources)
nodist_libbse_@MAJOR@_la_SOURCES     = gslfft.cc
libbse_@MAJOR@_la_CXXFLAGS	     = $(AM_CXXFLAGS) $(BSE_CFLAGS) -DBSE_CONVENIENCE
libbse_@MAJOR@_la_LIBADD             = $(top_builddir)/sfi/libsfi-@MAJOR@.la $(BSE_LIBS)
libbse_@MAJOR@_la_LDFLAGS            = $(strip		\
	$(AM_LDFLAGS)					\
	-Wl,--version-script=$(srcdir)/ldscript.map	\
	-version-info $(LT_VERSION_INFO)		\
)
#	-Wl,-Bsymbolic
# setup sources, their dependancies and commands
CLEANFILES += $(idl_built_sources)
EXTRA_HEADERS +=
GLIB_MKENUMS = glib-mkenums
# WATCH OUT: $(SFIDL) works only builddir relative
SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_builddir) -I$(top_srcdir) -I$(srcdir)

install-exec-ldconfig: # libtool >= 1.5.2 doesn't always set rpath, requiring ldconfig
	test ! -x /sbin/ldconfig -o ! -w /etc/ld.so.cache || /sbin/ldconfig $(DESTDIR)$(bselibdir)
install-exec-hook: install-exec-ldconfig

# == gslfft.cc ==
GENERATED_CLEANFILES += gslfft.cc
$(srcdir)/gslfft.cc: @PERLRULE@ $(srcdir)/gsl-fftgen.pl $(srcdir)/gsl-fftconf.sh
	$(AM_V_GEN)
	$(Q) (cd . \
	&& echo '#pragma GCC diagnostic ignored "-Wunused-but-set-variable"' \
	&& $(srcdir)/gsl-fftconf.sh '$(PERL) $(srcdir)/gsl-fftgen.pl $(if $(findstring 1, $(V)) ,, --no-verbose)' \"gslfft.hh\" \
	) > xgen-$(@F) \
	&& mv xgen-$(@F) $@

# == bsegentypes.h ==
mktypes_bse_sources = $(filter-out bsegentypes.cc, $(bse_sources))
GENERATED_CLEANFILES += bsegentypes.h
bsetype.hh $(bse_sources): bsegentypes.h	# $(libbse_@MAJOR@_la_OBJECTS): bsegentypes.h
bsegentypes.h: @PERLRULE@ mktypes.pl # $(filter-out bsegentypes.cc, $(bse_sources)) $(bse_public_headers)
bsegentypes.h: @PERLRULE@ bsebasics.idl # $(SFIDL)
	$(AM_V_GEN)
	$(Q) ( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */" \
	    --eprod "#define BSE_TYPE_@ENUMSHORT@\t    (BSE_TYPE_ID (@EnumName@)) // enum\n" \
	    --eprod "extern GType BSE_TYPE_ID (@EnumName@);" $(bse_public_headers) \
	  && $(PERL) mktypes.pl --externs $(mktypes_bse_sources) \
	) > xgen-$(@F) \
	&& $(SFIDL) $(SFIDL_INC) --core-c --header $(srcdir)/bsebasics.idl >> xgen-$(@F) \
	&& mv xgen-$(@F) $(srcdir)/bsegentypes.h
# bsegentypes.cc
GENERATED_CLEANFILES += bsegentypes.cc
$(libbse_@MAJOR@_la_OBJECTS): bsegentypes.cc
bsegentypes.cc: @PERLRULE@ bsegentypes.h mktypes.pl bsebasics.idl # $(SFIDL)
	$(AM_V_GEN)
	$(Q) ( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --eprod "\nGType BSE_TYPE_ID (@EnumName@) = 0;" $(bse_public_headers) \
	  && $(PERL) mktypes.pl --interns --export-proto $(mktypes_bse_sources) \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# bseenum_arrays.cc
GENERATED_CLEANFILES += bseenum_arrays.cc
$(libbse_@MAJOR@_la_OBJECTS): bseenum_arrays.cc
bseenum_arrays.cc: bsegentypes.h
	$(AM_V_GEN)
	$(Q) ( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	    --vhead "/* @EnumName@\n */\n" \
	    --vhead "static G@Type@Value @enum_name@_values[] = { // enum_values\n" \
	    --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	    --vtail "  { 0, NULL, NULL }\n};\n" $(bse_public_headers) \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bseenum_list.cc
GENERATED_CLEANFILES += bseenum_list.cc
$(libbse_@MAJOR@_la_OBJECTS): bseenum_list.cc
bseenum_list.cc: bsegentypes.h
	$(AM_V_GEN)
	$(Q) ( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */" \
	    --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &BSE_TYPE_ID (@EnumName@), @enum_name@_values }," \
	      $(bse_public_headers) \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bsegentype_array.cc
GENERATED_CLEANFILES += bsegentype_array.cc
$(libbse_@MAJOR@_la_OBJECTS): bsegentype_array.cc
bsegentype_array.cc: @PERLRULE@ bsegentypes.h
	$(AM_V_GEN)
	$(Q) cd $(srcdir) \
	&& $(PERL) mktypes.pl --array $(mktypes_bse_sources) > xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# bsegenbasics.cc
GENERATED_CLEANFILES += bsegenbasics.cc
$(srcdir)/bsebasics.cc: bsegenbasics.cc
bsegenbasics.cc: bsebasics.idl bsebasics.genidl.hh # $(SFIDL)
	$(AM_V_GEN)
	$(Q) cd . \
	&& $(SFIDL) $(SFIDL_INC) --core-c --source --init sfidl_types_init $(srcdir)/bsebasics.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $(@F) \
	&& rm -f xgen-$(@F)
# setup generation of C++ objects from idl files
%.genidl.hh: $(srcdir)/%.idl # $(SFIDL)
	$(AM_V_GEN)
	$(Q) $(SFIDL) --core-cxx --macro $(<F) $(SFIDL_INC) $< > xgen-$(@F)
	$(Q) mv xgen-$(@F) $@

# == bsehack.idl ==
# currently generated from BSE introspection data, needed to build the old IDL bindings
noinst_DATA = bsehack.idl
bsehack.idl: stamp-bhi	# generate indirectly via timestamp file to avoid excessive rebuilds
	@test -f $@ || rm -f stamp-bhi
	@test -f $@ || $(MAKE) $(AM_MAKEFLAGS) stamp-bhi
CLEANFILES += bsehack.idl stamp-bhi
stamp-bhi: bse.idl bsebasics.idl bsecxxbase.idl bsecxxmodule.idl bseprocidl # real deps for bsehack.idl
	@echo "  GEN     " bsehack.idl
	$(Q) echo "/* file generated by make bsehack.idl */" 	 > xgen-bhi1
	$(Q) grep -v 'include.*bsehack.idl' $(srcdir)/bse.idl | $(SFIDL) $(SFIDL_INC) --list-types - > xgen-bhi2
	$(Q) ./bseprocidl xgen-bhi2 --g-fatal-warnings 		>> xgen-bhi1
	$(Q) cmp xgen-bhi1 bsehack.idl 2>/dev/null || mv xgen-bhi1 bsehack.idl
	$(Q) rm -f xgen-bhi1 xgen-bhi2 && touch $@
CLEANFILES += bsehack.idl

# == bseapi.idl ==
AUXTYPES_PY = $(topdir)/bse/AuxTypes.py
../bse/%_interfaces.hh ../bse/%_interfaces.cc ../bse/%_handles.hh ../bse/%_handles.cc ../bse/.%.stamp \
%_interfaces.hh %_interfaces.cc %_handles.hh %_handles.cc .%.stamp: %.idl bseapi-inserts.hh $(AIDACC) $(AIDACC_CXXSTUB_PY) $(AUXTYPES_PY)
	$(QGEN) # MAKE(1) supports parallel execution of a multiple-outputs-rule *only* for pattern rules (with '%')
	$(Q) $(AIDACC) -x CxxStub -x $(AUXTYPES_PY) -G strip-path=$(topdir)/ --insertions $(srcdir)/bseapi-inserts.hh $<
	$(Q) sed -i -e '1i#define _(x) x' bseapi_interfaces.cc && sed -i -e '1i#undef _' bseapi_interfaces.cc
	$(Q) touch .bseapi.stamp # memorize completion without errors
CLEANFILES += bseapi_interfaces.hh bseapi_interfaces.cc bseapi_handles.hh bseapi_handles.cc .bseapi.stamp
$(srcdir)/bseutils.cc: bseapi_interfaces.hh bseapi_interfaces.cc .bseapi.stamp
$(srcdir)/bsestartup.cc: bseapi_handles.hh bseapi_handles.cc .bseapi.stamp
$(libbse_@MAJOR@_la_OBJECTS): .bseapi.stamp	# needed dependency to force file generation

# == GENERATED targets ==
CLEANFILES += $(GENERATED)
$(libbse_@MAJOR@_la_OBJECTS): $(GENERATED)	# needed dependency to force file generation
$(bseprocidl_OBJECTS) $(bsetool_OBJECTS): $(GENERATED)

#
# private (birnet) dependancy to make TAGS file after each build
#
all: # TAGS



#
# other programs, we want to compile
#
noinst_PROGRAMS = $(ALLTESTS)
progs_LDADD     = libbse-@MAJOR@.la $(BSE_LIBS)

noinst_PROGRAMS    += bseprocidl
bseprocidl_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS) -DBSE_CONVENIENCE
bseprocidl_LDADD    = $(progs_LDADD)
bseprocidl_SOURCES  = bseprocidl.cc

# == bsetool ==
noinst_PROGRAMS += bsetool
bsetool_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS) -DBSE_CONVENIENCE
bsetool_LDADD    = $(progs_LDADD)
bsetool_SOURCES  = bsetool.cc

# == Tests ==
check-oldidl: oldidl.idl AuxTypes.py
	@echo "  CHECK " $<
	$(Q) $(AIDACC_VDEBUG) --bse-extensions -x $(srcdir)/AuxTypes.py $<
.PHONY: check-oldidl
check-local: check-oldidl
