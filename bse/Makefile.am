# BEAST & BSE

include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)

SUBDIRS = icons . tests

AM_CPPFLAGS += -I../sfi/
DEFS        += -DG_LOG_DOMAIN=\"BSE\" -DG_DISABLE_CONST_RETURNS -DBSE_COMPILATION
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
	backtrace.hh			\
	bcore.hh			\
	blob.hh				\
	bse.hh				\
	bsebiquadfilter.hh		\
	bseblockutils.hh		\
	bsebus.hh			\
	bsecategories.hh		\
	bsecompat.hh			\
	bseconfig.h			\
	bseconstant.hh			\
	bseconstvalues.hh		\
	bsecontainer.hh			\
	bsecontextmerger.hh		\
	bsecsynth.hh			\
	bsecxxarg.hh			\
	bsecxxbase.hh			\
	bsecxxclosure.hh		\
	bsecxxmodule.hh			\
	bsecxxplugin.hh			\
	bsecxxutils.hh			\
	bsecxxvalue.hh			\
	bsedatahandle-flac.hh		\
	bsedefs.hh			\
	bsedevice.hh			\
	bseeditablesample.hh		\
	bseengine.hh			\
	bseenginemaster.hh		\
	bseenginenode.hh		\
	bseengineprivate.hh		\
	bseengineschedule.hh		\
	bseengineutils.hh		\
	bseenums.hh			\
	bseexports.hh			\
	bsefilter.hh			\
	bsegconfig.hh			\
	bsegenclosures.hh		\
	bseglobals.hh			\
	bseglue.hh			\
	bseieee754.hh			\
	bseincluder.hh			\
	bseinstrumentinput.hh		\
	bseinstrumentoutput.hh		\
	bseitem.hh			\
	bseladspa.hh			\
	bseladspamodule.hh		\
	bseloader.hh			\
	bsemain.hh			\
	bsemath.hh			\
	bsemathsignal.hh		\
	bsemidicontroller.hh		\
	bsemididecoder.hh		\
	bsemididevice-null.hh		\
	bsemididevice-oss.hh		\
	bsemididevice.hh		\
	bsemidievent.hh			\
	bsemidifile.hh			\
	bsemidiinput.hh			\
	bsemidinotifier.hh		\
	bsemidireceiver.hh		\
	bsemidisynth.hh			\
	bsemidivoice.hh			\
	bsenote.hh			\
	bseobject.hh			\
	bseparam.hh			\
	bsepart.hh			\
	bsepcmdevice-null.hh		\
	bsepcmdevice-oss.hh		\
	bsepcmdevice.hh			\
	bsepcminput.hh			\
	bsepcmoutput.hh			\
	bsepcmwriter.hh			\
	bseplugin.hh			\
	bseproject.hh			\
	bseresampler.hh			\
	bseresamplerimpl.hh		\
	bsesequencer.hh			\
	bseserver.hh			\
	bsesnet.hh			\
	bsesnooper.hh			\
	bsesong.hh			\
	bsesoundfont.hh			\
	bsesoundfontosc.hh		\
	bsesoundfontpreset.hh		\
	bsesoundfontrepo.hh		\
	bsesource.hh			\
	bsestandardosc.hh		\
	bsestandardsynths.hh		\
	bsestartup.hh			\
	bsestorage.hh			\
	bsesubiport.hh			\
	bsesuboport.hh			\
	bsesubsynth.hh			\
	bsesuper.hh			\
	bsetrack.hh			\
	bsetype.hh			\
	bseundostack.hh			\
	bseutils.hh			\
	bsewave.hh			\
	bsewaveosc.hh			\
	bsewaverepo.hh			\
	cxxaux.hh			\
	datalist.hh			\
	effectbase.hh			\
	entropy.hh			\
	formatter.hh			\
	gbsearcharray.hh		\
	glib-extra.hh			\
	gslcommon.hh			\
	gsldatacache.hh			\
	gsldatahandle-mad.hh		\
	gsldatahandle-vorbis.hh		\
	gsldatahandle.hh		\
	gsldatautils.hh			\
	gsldefs.hh			\
	gslfft.hh			\
	gslfilehash.hh			\
	gslfilter.hh			\
	gslmagic.hh			\
	gsloscillator.hh		\
	gslosctable.hh			\
	gslvorbis-cutter.hh		\
	gslvorbis-enc.hh		\
	gslwavechunk.hh			\
	gslwaveosc.hh			\
	internal.hh			\
	ladspa.hh			\
	memory.hh			\
	monitor.hh			\
	path.hh				\
	platform.hh			\
	randomhash.hh			\
	sfi.hh				\
	sficomport.hh			\
	sficomwire.hh			\
	sficxx.hh			\
	sfifilecrawler.hh		\
	sfiglue.hh			\
	sfigluecodec.hh			\
	sfiglueproxy.hh			\
	sfimemory.hh			\
	sfinote.hh			\
	sfiparams.hh			\
	sfiprimitives.hh		\
	sfiring.hh			\
	sfiserial.hh			\
	sfistore.hh			\
	sfitime.hh			\
	sfitypes.hh			\
	sfiustore.hh			\
	sfivalues.hh			\
	sfivisitors.hh			\
	sfivmarshal.hh			\
	sfiwrapper.hh			\
	strings.hh			\
	testing.hh			\
	testobject.hh			\
	unicode.hh			\
)
# BSE C & C++ sources
bse_sources = $(strip \
	backtrace.cc			\
	bcore.cc			\
	blob.cc				\
	bsebasics.cc			\
	bsebiquadfilter.cc		\
	bseblockutils.cc		\
	bsebus.cc			\
	bsebusmodule.cc			\
	bsecategories.cc		\
	bsecompat.cc			\
	bseconstant.cc			\
	bseconstvalues.cc		\
	bsecontainer.cc			\
	bsecontextmerger.cc		\
	bsecsynth.cc			\
	bsecxxarg.cc			\
	bsecxxbase.cc			\
	bsecxxclosure.cc		\
	bsecxxmodule.cc			\
	bsecxxplugin.cc			\
	bsecxxutils.cc			\
	bsecxxvalue.cc			\
	bsedatahandle-fir.cc		\
	bsedatahandle-flac.cc		\
	bsedatahandle-resample.cc	\
	bsedevice.cc			\
	bseeditablesample.cc		\
	bseengine.cc			\
	bseenginemaster.cc		\
	bseengineschedule.cc		\
	bseengineutils.cc		\
	bseenums.cc			\
	bsefilter-ellf.cc		\
	bsefilter.cc			\
	bsegconfig.cc			\
	bseglobals.cc			\
	bseglue.cc			\
	bseinstrumentinput.cc		\
	bseinstrumentoutput.cc		\
	bseitem.cc			\
	bseladspa.cc			\
	bseladspamodule.cc		\
	bseloader-aiff.cc		\
	bseloader-bsewave.cc		\
	bseloader-flac.cc		\
	bseloader-guspatch.cc		\
	bseloader-mad.cc		\
	bseloader-oggvorbis.cc		\
	bseloader-wav.cc		\
	bseloader.cc			\
	bsemain.cc			\
	bsemath.cc			\
	bsemathsignal.cc		\
	bsemidicontroller.cc		\
	bsemididecoder.cc		\
	bsemididevice-null.cc		\
	bsemididevice-oss.cc		\
	bsemididevice.cc		\
	bsemidievent.cc			\
	bsemidifile.cc			\
	bsemidiinput.cc			\
	bsemidinotifier.cc		\
	bsemidireceiver.cc		\
	bsemidisynth.cc			\
	bsemidivoice.cc			\
	bsenote.cc			\
	bseobject.cc			\
	bseparam.cc			\
	bsepart.cc			\
	bsepcmdevice-null.cc		\
	bsepcmdevice-oss.cc		\
	bsepcmdevice.cc			\
	bsepcminput.cc			\
	bsepcmoutput.cc			\
	bsepcmwriter.cc			\
	bseplugin.cc			\
	bseproject.cc			\
	bseresampler.cc			\
	bsesequencer.cc			\
	bseserver.cc			\
	bsesnet.cc			\
	bsesnooper.cc			\
	bsesong.cc			\
	bsesoundfont.cc			\
	bsesoundfontosc.cc		\
	bsesoundfontpreset.cc		\
	bsesoundfontrepo.cc		\
	bsesource.cc			\
	bsestandardosc.cc		\
	bsestandardsynths.cc		\
	bsestartup.cc			\
	bsestorage.cc			\
	bsesubiport.cc			\
	bsesuboport.cc			\
	bsesubsynth.cc			\
	bsesuper.cc			\
	bsetrack.cc			\
	bsetype.cc			\
	bseundostack.cc			\
	bseutils.cc			\
	bsewave.cc			\
	bsewaveosc.cc			\
	bsewaverepo.cc			\
	datalist.cc			\
	entropy.cc			\
	formatter.cc			\
	gslcommon.cc			\
	gsldatacache.cc			\
	gsldatahandle-mad.cc		\
	gsldatahandle-vorbis.cc		\
	gsldatahandle.cc		\
	gsldatautils.cc			\
	gslfilehash.cc			\
	gslfilter.cc			\
	gslmagic.cc			\
	gsloscillator.cc		\
	gslosctable.cc			\
	gslvorbis-cutter.cc		\
	gslvorbis-enc.cc		\
	gslwavechunk.cc			\
	gslwaveosc.cc			\
	memory.cc			\
	monitor.cc			\
	path.cc				\
	platform.cc			\
	randomhash.cc			\
	sficomport.cc			\
	sficomwire.cc			\
	sfifilecrawler.cc		\
	sfiglue.cc			\
	sfigluecodec.cc			\
	sfiglueproxy.cc			\
	sfimemory.cc			\
	sfinote.cc			\
	sfiparams.cc			\
	sfiprimitives.cc		\
	sfiring.cc			\
	sfiserial.cc			\
	sfistore.cc			\
	sfitime.cc			\
	sfitypes.cc			\
	sfiustore.cc			\
	sfivalues.cc			\
	sfivisitors.cc			\
	sfivmarshal.cc			\
	sfiwrapper.cc			\
	strings.cc			\
	testing.cc			\
	testobject.cc			\
	unicode.cc			\
	weaksym.cc			\
)
# glib-extra.cc gets compiled as part of bseconstvalues.cc
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
	bsecxxbase.idl		\
	bsecxxmodule.idl	\
)

doc_sources = $(bse_idl_sources) $(bse_sources) $(bse_public_headers) sysconfig.h bseapi.idl

# == doc_sources.lst ==
doc_sources.lst: Makefile
	$(AM_V_GEN)
	$(Q) echo $(patsubst %, bse/%, $(doc_sources)) > $@
CLEANFILES += doc_sources.lst

#
# BSE library
#
bselibexecdir			     = $(bselibdir)
bselibexec_LTLIBRARIES               = libbse-@MAJOR@.la
libbse_@MAJOR@includedir             = $(bseincludedir)/bse
libbse_@MAJOR@include_HEADERS        = $(bse_public_headers) sysconfig.h bse.idl $(bse_idl_sources) $(idl_dummy_files)
nodist_libbse_@MAJOR@include_HEADERS = bsebasics.genidl.hh bseapi_interfaces.hh bseapi_handles.hh bsehack.idl bsegentypes.h
libbse_@MAJOR@_la_SOURCES            = $(bse_sources)
nodist_libbse_@MAJOR@_la_SOURCES     = gslfft.cc
libbse_@MAJOR@_la_CXXFLAGS	     = $(AM_CXXFLAGS) $(BSE_CFLAGS)
libbse_@MAJOR@_la_LIBADD             = $(BSE_LIBS)
libbse_@MAJOR@_la_LDFLAGS            = $(strip		\
	$(AM_LDFLAGS)					\
	-Wl,--version-script=$(srcdir)/ldscript.map	\
	-version-info $(LT_VERSION_INFO)		\
)
#	-Wl,-Bsymbolic
# split off debug info
SED_SONAME = sed -n "/^library_names='/{ s/^[^']*'\([^ ']*\).*/\1/; p; q }"
install-exec-debug:
	mkdir -p "$(DESTDIR)$(bselibexecdir)/.debug/"
	for lib in $(bselibexec_LTLIBRARIES) ; do \
	  soname=$$($(SED_SONAME) "$$lib") && \
	  objcopy --only-keep-debug "$(DESTDIR)$(bselibexecdir)/$$soname" "$(DESTDIR)$(bselibexecdir)/.debug/$$soname.debug" && \
	  objcopy --strip-debug --add-gnu-debuglink="$(DESTDIR)$(bselibexecdir)/.debug/$$soname.debug" "$(DESTDIR)$(bselibexecdir)/$$soname" \
	  || { echo "$$0: objcopy failed for: $(DESTDIR)$(bselibexecdir)/$$soname" >&2; exit 2; }; \
	done
uninstall-split-debug: # needs to run before 'uninstall' deletes $(bselibexec_LTLIBRARIES) from $(DESTDIR)$(bselibexecdir)/
	for lib in $(bselibexec_LTLIBRARIES) ; do \
	  test -e "$(DESTDIR)$(bselibexecdir)/$$lib" || continue; \
	  soname=$$($(SED_SONAME) "$(DESTDIR)$(bselibexecdir)/$$lib") && \
	  rm -f "$(DESTDIR)$(bselibexecdir)/.debug/$$soname.debug" ; \
	done
install-exec-hook: install-exec-debug
pre_uninstall_bselibexec_hook := $(strip uninstall-bselibexecLTLIBRARIES) # hook to run before and keep automake uninstall-bselibexecLTLIBRARIES
$(pre_uninstall_bselibexec_hook): uninstall-split-debug # run uninstall-split-debug *before* make 'uninstall'
# setup sources, their dependancies and commands
$(bse_public_headers): sysconfig.h
CLEANFILES += $(idl_built_sources)
EXTRA_HEADERS +=
GLIB_MKENUMS = glib-mkenums
# WATCH OUT: $(SFIDL) works only builddir relative
SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = -I$(top_builddir) -I$(top_srcdir) -I$(srcdir)

install-exec-ldconfig: # libtool >= 1.5.2 doesn't always set rpath, requiring ldconfig
	test ! -x /sbin/ldconfig -o ! -w /etc/ld.so.cache || /sbin/ldconfig $(DESTDIR)$(bselibexecdir)
install-exec-hook: install-exec-ldconfig

# == zres.cc ==
include ../res/Makefile.sub
# ^^^^^ FIXME: the above should only be included by the toplevel Makefile.am
MISC_PACKRES = $(top_srcdir)/misc/packres.py
zres.cc: $(top_srcdir)/res/resfiles.list $(MISC_PACKRES) # res_resfiles_list contains res/resfiles.list
	$(AM_V_GEN)
	$(Q) $(MISC_PACKRES) -s '.*/res/' $(res_resfiles_list:%=$(top_srcdir)/res/%) > $@.tmp
	$(Q) mv $@.tmp $@
CLEANFILES += zres.cc
bcore.cc: zres.cc

# == gslfft.cc ==
GENERATED_CLEANFILES += gslfft.cc
$(srcdir)/gslfft.cc: @PERLRULE@ $(srcdir)/gsl-fftgen.pl $(srcdir)/gsl-fftconf.sh
	$(AM_V_GEN)
	$(Q) (cd . \
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
bseenum_arrays.cc: bsegentypes.h $(bse_public_headers)
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
bseenum_list.cc: bsegentypes.h $(bse_public_headers)
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
bseprocidl_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS)
bseprocidl_LDADD    = $(progs_LDADD)
bseprocidl_SOURCES  = bseprocidl.cc

# == bsetool ==
noinst_PROGRAMS += bsetool
bsetool_CXXFLAGS = $(AM_CXXFLAGS) $(BSE_CFLAGS)
bsetool_LDADD    = $(progs_LDADD)
bsetool_SOURCES  = bsetool.cc

# == Tests ==
check-oldidl: oldidl.idl AuxTypes.py
	@echo "  CHECK " $<
	$(Q) $(AIDACC_VDEBUG) --bse-extensions -x $(srcdir)/AuxTypes.py $<
.PHONY: check-oldidl
check-local: check-oldidl
