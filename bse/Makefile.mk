# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/bse/*.d)
CLEANDIRS += $(wildcard $>/bse/)

include bse/icons/Makefile.mk

# == libbse.so ==
bse/libbse.headers ::= $(strip		\
	bse/backtrace.hh		\
	bse/bcore.hh			\
	bse/blob.hh			\
	bse/bse.hh			\
	bse/bsebiquadfilter.hh		\
	bse/bseblockutils.hh		\
	bse/bsebus.hh			\
	bse/bsecategories.hh		\
	bse/bsecompat.hh		\
	bse/bseconstant.hh		\
	bse/bseconstvalues.hh		\
	bse/bsecontainer.hh		\
	bse/bsecontextmerger.hh		\
	bse/bsecsynth.hh		\
	bse/bsecxxarg.hh		\
	bse/bsecxxbase.hh		\
	bse/bsecxxclosure.hh		\
	bse/bsecxxmodule.hh		\
	bse/bsecxxplugin.hh		\
	bse/bsecxxutils.hh		\
	bse/bsecxxvalue.hh		\
	bse/bsedatahandle-flac.hh	\
	bse/bsedefs.hh			\
	bse/bsedevice.hh		\
	bse/bseeditablesample.hh	\
	bse/bseengine.hh		\
	bse/bseenginemaster.hh		\
	bse/bseenginenode.hh		\
	bse/bseengineprivate.hh		\
	bse/bseengineschedule.hh	\
	bse/bseengineutils.hh		\
	bse/bseenums.hh			\
	bse/bseexports.hh		\
	bse/bsefilter.hh		\
	bse/bsegconfig.hh		\
	bse/bsegenclosures.hh		\
	bse/bseglobals.hh		\
	bse/bseglue.hh			\
	bse/bseieee754.hh		\
	bse/bseincluder.hh		\
	bse/bseinstrumentinput.hh	\
	bse/bseinstrumentoutput.hh	\
	bse/bseitem.hh			\
	bse/bseladspa.hh		\
	bse/bseladspamodule.hh		\
	bse/bseloader.hh		\
	bse/bsemain.hh			\
	bse/bsemath.hh			\
	bse/bsemathsignal.hh		\
	bse/bsemidicontroller.hh	\
	bse/bsemididecoder.hh		\
	bse/bsemididevice-null.hh	\
	bse/bsemididevice-oss.hh	\
	bse/bsemididevice.hh		\
	bse/bsemidievent.hh		\
	bse/bsemidifile.hh		\
	bse/bsemidiinput.hh		\
	bse/bsemidinotifier.hh		\
	bse/bsemidireceiver.hh		\
	bse/bsemidisynth.hh		\
	bse/bsemidivoice.hh		\
	bse/bsenote.hh			\
	bse/bseobject.hh		\
	bse/bseparam.hh			\
	bse/bsepart.hh			\
	bse/bsepcmdevice-null.hh	\
	bse/bsepcmdevice-oss.hh		\
	bse/bsepcmdevice.hh		\
	bse/bsepcminput.hh		\
	bse/bsepcmoutput.hh		\
	bse/bsepcmwriter.hh		\
	bse/bseplugin.hh		\
	bse/bseproject.hh		\
	bse/bseresampler.hh		\
	bse/bseresamplerimpl.hh		\
	bse/bsesequencer.hh		\
	bse/bseserver.hh		\
	bse/bsesnet.hh			\
	bse/bsesnooper.hh		\
	bse/bsesong.hh			\
	bse/bsesoundfont.hh		\
	bse/bsesoundfontosc.hh		\
	bse/bsesoundfontpreset.hh	\
	bse/bsesoundfontrepo.hh		\
	bse/bsesource.hh		\
	bse/bsestandardosc.hh		\
	bse/bsestandardsynths.hh	\
	bse/bsestartup.hh		\
	bse/bsestorage.hh		\
	bse/bsesubiport.hh		\
	bse/bsesuboport.hh		\
	bse/bsesubsynth.hh		\
	bse/bsesuper.hh			\
	bse/bsetrack.hh			\
	bse/bsetype.hh			\
	bse/bseundostack.hh		\
	bse/bseutils.hh			\
	bse/bsewave.hh			\
	bse/bsewaveosc.hh		\
	bse/bsewaverepo.hh		\
	bse/cxxaux.hh			\
	bse/datalist.hh			\
	bse/effectbase.hh		\
	bse/entropy.hh			\
	bse/formatter.hh		\
	bse/gbsearcharray.hh		\
	bse/glib-extra.hh		\
	bse/gslcommon.hh		\
	bse/gsldatacache.hh		\
	bse/gsldatahandle-mad.hh	\
	bse/gsldatahandle-vorbis.hh	\
	bse/gsldatahandle.hh		\
	bse/gsldatautils.hh		\
	bse/gsldefs.hh			\
	bse/gslfft.hh			\
	bse/gslfilehash.hh		\
	bse/gslfilter.hh		\
	bse/gslmagic.hh			\
	bse/gsloscillator.hh		\
	bse/gslosctable.hh		\
	bse/gslvorbis-cutter.hh		\
	bse/gslvorbis-enc.hh		\
	bse/gslwavechunk.hh		\
	bse/gslwaveosc.hh		\
	bse/internal.hh			\
	bse/ladspa.hh			\
	bse/memory.hh			\
	bse/monitor.hh			\
	bse/path.hh			\
	bse/platform.hh			\
	bse/randomhash.hh		\
	bse/sfi.hh			\
	bse/sficomport.hh		\
	bse/sficomwire.hh		\
	bse/sficxx.hh			\
	bse/sfifilecrawler.hh		\
	bse/sfiglue.hh			\
	bse/sfigluecodec.hh		\
	bse/sfiglueproxy.hh		\
	bse/sfimemory.hh		\
	bse/sfinote.hh			\
	bse/sfiparams.hh		\
	bse/sfiprimitives.hh		\
	bse/sfiring.hh			\
	bse/sfiserial.hh		\
	bse/sfistore.hh			\
	bse/sfitime.hh			\
	bse/sfitypes.hh			\
	bse/sfiustore.hh		\
	bse/sfivalues.hh		\
	bse/sfivisitors.hh		\
	bse/sfivmarshal.hh		\
	bse/sfiwrapper.hh		\
	bse/strings.hh			\
	bse/testing.hh			\
	bse/testobject.hh		\
	bse/unicode.hh			\
)
bse/libbse.sources ::= $(strip		\
	bse/backtrace.cc		\
	bse/bcore.cc			\
	bse/blob.cc			\
	bse/bsebasics.cc		\
	bse/bsebiquadfilter.cc		\
	bse/bseblockutils.cc		\
	bse/bsebus.cc			\
	bse/bsebusmodule.cc		\
	bse/bsecategories.cc		\
	bse/bsecompat.cc		\
	bse/bseconstant.cc		\
	bse/bseconstvalues.cc		\
	bse/bsecontainer.cc		\
	bse/bsecontextmerger.cc		\
	bse/bsecsynth.cc		\
	bse/bsecxxarg.cc		\
	bse/bsecxxbase.cc		\
	bse/bsecxxclosure.cc		\
	bse/bsecxxmodule.cc		\
	bse/bsecxxplugin.cc		\
	bse/bsecxxutils.cc		\
	bse/bsecxxvalue.cc		\
	bse/bsedatahandle-fir.cc	\
	bse/bsedatahandle-flac.cc	\
	bse/bsedatahandle-resample.cc	\
	bse/bsedevice.cc		\
	bse/bseeditablesample.cc	\
	bse/bseengine.cc		\
	bse/bseenginemaster.cc		\
	bse/bseengineschedule.cc	\
	bse/bseengineutils.cc		\
	bse/bseenums.cc			\
	bse/bsefilter-ellf.cc		\
	bse/bsefilter.cc		\
	bse/bsegconfig.cc		\
	bse/bseglobals.cc		\
	bse/bseglue.cc			\
	bse/bseinstrumentinput.cc	\
	bse/bseinstrumentoutput.cc	\
	bse/bseitem.cc			\
	bse/bseladspa.cc		\
	bse/bseladspamodule.cc		\
	bse/bseloader-aiff.cc		\
	bse/bseloader-bsewave.cc	\
	bse/bseloader-flac.cc		\
	bse/bseloader-guspatch.cc	\
	bse/bseloader-mad.cc		\
	bse/bseloader-oggvorbis.cc	\
	bse/bseloader-wav.cc		\
	bse/bseloader.cc		\
	bse/bsemain.cc			\
	bse/bsemath.cc			\
	bse/bsemathsignal.cc		\
	bse/bsemidicontroller.cc	\
	bse/bsemididecoder.cc		\
	bse/bsemididevice-null.cc	\
	bse/bsemididevice-oss.cc	\
	bse/bsemididevice.cc		\
	bse/bsemidievent.cc		\
	bse/bsemidifile.cc		\
	bse/bsemidiinput.cc		\
	bse/bsemidinotifier.cc		\
	bse/bsemidireceiver.cc		\
	bse/bsemidisynth.cc		\
	bse/bsemidivoice.cc		\
	bse/bsenote.cc			\
	bse/bseobject.cc		\
	bse/bseparam.cc			\
	bse/bsepart.cc			\
	bse/bsepcmdevice-null.cc	\
	bse/bsepcmdevice-oss.cc		\
	bse/bsepcmdevice.cc		\
	bse/bsepcminput.cc		\
	bse/bsepcmoutput.cc		\
	bse/bsepcmwriter.cc		\
	bse/bseplugin.cc		\
	bse/bseproject.cc		\
	bse/bseresampler.cc		\
	bse/bsesequencer.cc		\
	bse/bseserver.cc		\
	bse/bsesnet.cc			\
	bse/bsesnooper.cc		\
	bse/bsesong.cc			\
	bse/bsesoundfont.cc		\
	bse/bsesoundfontosc.cc		\
	bse/bsesoundfontpreset.cc	\
	bse/bsesoundfontrepo.cc		\
	bse/bsesource.cc		\
	bse/bsestandardosc.cc		\
	bse/bsestandardsynths.cc	\
	bse/bsestartup.cc		\
	bse/bsestorage.cc		\
	bse/bsesubiport.cc		\
	bse/bsesuboport.cc		\
	bse/bsesubsynth.cc		\
	bse/bsesuper.cc			\
	bse/bsetrack.cc			\
	bse/bsetype.cc			\
	bse/bseundostack.cc		\
	bse/bseutils.cc			\
	bse/bsewave.cc			\
	bse/bsewaveosc.cc		\
	bse/bsewaverepo.cc		\
	bse/datalist.cc			\
	bse/entropy.cc			\
	bse/formatter.cc		\
	bse/gslcommon.cc		\
	bse/gsldatacache.cc		\
	bse/gsldatahandle-mad.cc	\
	bse/gsldatahandle-vorbis.cc	\
	bse/gsldatahandle.cc		\
	bse/gsldatautils.cc		\
	bse/gslfilehash.cc		\
	bse/gslfilter.cc		\
	bse/gslmagic.cc			\
	bse/gsloscillator.cc		\
	bse/gslosctable.cc		\
	bse/gslvorbis-cutter.cc		\
	bse/gslvorbis-enc.cc		\
	bse/gslwavechunk.cc		\
	bse/gslwaveosc.cc		\
	bse/memory.cc			\
	bse/monitor.cc			\
	bse/path.cc			\
	bse/platform.cc			\
	bse/randomhash.cc		\
	bse/sficomport.cc		\
	bse/sficomwire.cc		\
	bse/sfifilecrawler.cc		\
	bse/sfiglue.cc			\
	bse/sfigluecodec.cc		\
	bse/sfiglueproxy.cc		\
	bse/sfimemory.cc		\
	bse/sfinote.cc			\
	bse/sfiparams.cc		\
	bse/sfiprimitives.cc		\
	bse/sfiring.cc			\
	bse/sfiserial.cc		\
	bse/sfistore.cc			\
	bse/sfitime.cc			\
	bse/sfitypes.cc			\
	bse/sfiustore.cc		\
	bse/sfivalues.cc		\
	bse/sfivisitors.cc		\
	bse/sfivmarshal.cc		\
	bse/sfiwrapper.cc		\
	bse/strings.cc			\
	bse/testing.cc			\
	bse/testobject.cc		\
	bse/unicode.cc			\
	bse/weaksym.cc			\
)
bse/libbse.generated::= bse/gslfft.cc
bse/libbse.deps     ::= $(strip		\
	$(bse/icons/c.csources)		\
	$>/bse/bseapi_interfaces.hh	\
	$>/bse/bsebasics.genidl.hh	\
	$>/bse/bsebusmodule.genidl.hh	\
	$>/bse/bseenum_arrays.cc	\
	$>/bse/bseenum_list.cc		\
	$>/bse/bsegenbasics.cc		\
	$>/bse/bsegentype_array.cc	\
	$>/bse/bsegentypes.cc		\
	$>/bse/bsegentypes.h		\
	$>/bse/sysconfig.h		\
	$>/bse/zres.cc			\
)
bse.so              ::= bse-$(VERSION_MAJOR).so
libbse.soname       ::= lib$(bse.so).$(VERSION_MINOR)
bse/libbse.sofile   ::= $>/bse/$(libbse.soname).$(VERSION_MICRO)
bse/libbse.solinks  ::= $>/bse/$(libbse.soname) $>/bse/lib$(bse.so)
ALL_TARGETS	     += $(bse/libbse.sofile) $(bse/libbse.solinks)
bse/libbse.objects ::= $(sort $(bse/libbse.sources:%.cc=$>/%.o) $(bse/libbse.generated:%.cc=$>/%.o))
$(bse/libbse.objects): $(bse/libbse.deps)
$(bse/libbse.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(bse/libbse.objects): EXTRA_DEFS ::= -DBSE_COMPILATION
# SO linking
$(bse/libbse.sofile): $(bse/libbse.objects) $(MAKEFILE_LIST)
	$(QECHO) LD $@
	$Q $(CXX) $(CXXSTD) -shared -fPIC -o $@ $(bse/libbse.objects) -Wl,-soname,$(libbse.soname) $(LDFLAGS) $(EXTRA_FLAGS) $($@.FLAGS) \
	$(GLIB_LIBS) `pkg-config --libs gobject-2.0 vorbisfile vorbisenc vorbis ogg mad fluidsynth gmodule-no-export-2.0 flac` -lz
$(bse/libbse.solinks): $(bse/libbse.sofile)
	$(QECHO) LN $@
	$Q rm -f $@ ; ln -s $(notdir $(bse/libbse.sofile)) $@


# == bsetool ==
bse/bsetool         ::= $>/bse/bsetool
ALL_TARGETS	     += $(bse/bsetool)
bse/bsetool.sources ::= bse/bsetool.cc
bse/bsetool.objects ::= $(sort $(bse/bsetool.sources:%.cc=$>/%.o))
bse/bsetool.deps    ::= $(bse/libbse.deps)
$(bse/bsetool.objects): $(bse/bsetool.deps)
$(bse/bsetool.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
# BIN linking
$(bse/bsetool): $(bse/bsetool.objects) $(bse/libbse.solinks) $(MAKEFILE_LIST)
	$(QECHO) LD $@
	$Q $(CXX) $(CXXSTD) -fPIC -o $@ $(bse/bsetool.objects) -Wl,--no-undefined \
	-Wl,-rpath='$$ORIGIN/../bse:$$ORIGIN/' -Wl,-L$>/bse/ -lbse-$(VERSION_MAJOR) \
	$(GLIB_LIBS) `pkg-config --libs vorbisfile vorbisenc vorbis ogg mad fluidsynth flac` -lz
# CUSTOMIZATIONS: bse/bsetool.cc.FLAGS = -O2   ||   $>/bse/bsetool.o.FLAGS = -O3    ||    $>/bse/bsetool.o: EXTRA_FLAGS = -O1


# == bseapi.idl ==
$>/bse/bseapi_interfaces.hh: bse/bseapi.idl	bse/bseapi-inserts.hh $(aidacc/aidacc) bse/AuxTypes.py	| $>/bse/
	$(QGEN) # aidacc generates %_interfaces.{hh|cc} %_handles.{hh|cc} from %.idl and MAKE(1) supports multiple-outputs *only* for pattern rules
	$Q cp bse/bseapi-inserts.hh $< bse/AuxTypes.py $>/bse/
	$Q cd $>/bse/ && $(abspath $(aidacc/aidacc)) -x CxxStub -x AuxTypes.py -G strip-path=$(abspath $>)/ --insertions bseapi-inserts.hh $(<F)
	$Q cd $>/bse/ && sed '1i#define _(x) x' -i bseapi_interfaces.cc && sed '1i#undef _' -i bseapi_interfaces.cc
$>/bse/bseapi_handles.hh $>/bse/bseapi_handles.cc $>/bse/bseapi_interfaces.cc: $>/bse/bseapi_interfaces.hh

# == sfidl generated files ==
$>/bse/bseenum_arrays.cc: $(bse/libbse.headers)		| $>/bse/
	$(QGEN)
	$Q $(GLIB_MKENUMS)	--fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
				--vhead "/* @EnumName@\n */\n" \
				--vhead "static G@Type@Value @enum_name@_values[] = { // enum_values\n" \
				--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
				--vtail "  { 0, NULL, NULL }\n};\n" \
				$(bse/libbse.headers)			> $@.tmp
	$Q mv $@.tmp $@
$>/bse/bseenum_list.cc: $(bse/libbse.headers)		| $>/bse/
	$(QGEN)
	$Q $(GLIB_MKENUMS)	--fprod "\n/* --- @filename@ --- */" \
				--eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &BSE_TYPE_ID (@EnumName@), @enum_name@_values }," \
				$(bse/libbse.headers)			> $@.tmp
	$Q mv $@.tmp $@
$>/bse/bsegentypes.h: bse/bsebasics.idl		$(bse/libbse.headers) bse/mktypes.pl $(sfi/sfidl) | $>/bse/
	$(QGEN)
	$Q $(GLIB_MKENUMS)	--fprod "\n/* --- @filename@ --- */" \
				--eprod "#define BSE_TYPE_@ENUMSHORT@\t (BSE_TYPE_ID (@EnumName@)) // enum\n" \
				--eprod "extern GType BSE_TYPE_ID (@EnumName@);" \
				$(bse/libbse.headers)			> $@.tmp
	$Q $(PERL) bse/mktypes.pl --externs $(bse/libbse.sources)	>>$@.tmp
	$Q $(sfi/sfidl) $(sfi/sfidl.includes)	--core-c --header $<	>>$@.tmp
	$Q mv $@.tmp $@
$>/bse/bsegentypes.cc: $(bse/libbse.headers) bse/mktypes.pl | $>/bse/	# $(bse/libbse.sources)
	$(QGEN)
	$Q $(GLIB_MKENUMS)	--eprod "\nGType BSE_TYPE_ID (@EnumName@) = 0;" \
				$(bse/libbse.headers)					> $@.tmp
	$Q $(PERL) bse/mktypes.pl --interns --export-proto $(bse/libbse.sources)	>>$@.tmp
	$Q mv $@.tmp $@
$>/bse/bsegentype_array.cc: $(bse/libbse.headers) bse/mktypes.pl | $>/bse/	# $(bse/libbse.sources)
	$(QGEN)
	$Q $(PERL) bse/mktypes.pl --array $(bse/libbse.sources)	> $@.tmp
	$Q mv $@.tmp $@
$>/bse/bsegenbasics.cc: bse/bsebasics.idl		$(sfi/sfidl) | $>/bse/
	$(QGEN)
	$Q $(sfi/sfidl) $(sfi/sfidl.includes)	--core-c --source --init sfidl_types_init $<	> $@.tmp
	$Q mv $@.tmp $@
$>/bse/bsebasics.genidl.hh: bse/bsebasics.idl		$(sfi/sfidl) | $>/bse/
	$(QGEN)
	$Q $(sfi/sfidl) $(sfi/sfidl.includes)	--core-cxx --macro $(<F) $<	> $@.tmp
	$Q mv $@.tmp $@
$>/bse/bsebusmodule.genidl.hh: bse/bsebusmodule.idl	$(sfi/sfidl) | $>/bse/
	$(QGEN)
	$Q $(sfi/sfidl) $(sfi/sfidl.includes)	--core-cxx --macro $(<F) $<	> $@.tmp
	$Q mv $@.tmp $@

# == zres.cc ==
$>/bse/zres.cc: res/resfiles.list misc/packres.py # $(res_resfiles_list) is set to the contents of res/resfiles.list
	$(QGEN)
	$Q misc/packres.py -s '.*/res/' $(res_resfiles_list:%=res/%) > $@.tmp
	$Q mv $@.tmp $@


# == bse/sysconfig.h ==
$>/bse/sysconfig.h: config-checks.mk $>/config-cache.mk bse/Makefile.mk | $>/bse/
	$(QGEN)
	$Q echo '// make $@'							> $@.tmp
	$Q echo "#define BSE_MAJOR_VERSION		($(VERSION_MAJOR))"	>>$@.tmp
	$Q echo "#define BSE_MINOR_VERSION		($(VERSION_MINOR))"	>>$@.tmp
	$Q echo "#define BSE_MICRO_VERSION		($(VERSION_MICRO))"	>>$@.tmp
	$Q echo '#define BSE_VERSION_DATE		"$(VERSION_DATE)"'	>>$@.tmp
	$Q echo '#define BSE_VERSION_BUILDID		"$(BUILDID)"'		>>$@.tmp
	$Q : $(file > $>/conftest_spinlock_initializer.c, $(conftest_spinlock_initializer.c)) \
	&& $(CC) -Wall $>/conftest_spinlock_initializer.c -pthread -o $>/conftest_spinlock_initializer \
	&& (cd $> && ./conftest_spinlock_initializer) \
	&& echo '#define BSE_SPINLOCK_INITIALIZER' "	$$(cat $>/conftest_spinlock_initializer.txt)"	>>$@.tmp \
	&& rm $>/conftest_spinlock_initializer.c $>/conftest_spinlock_initializer $>/conftest_spinlock_initializer.txt
	$Q mv $@.tmp $@
# conftest_spinlock_initializer.c
define conftest_spinlock_initializer.c
#include <stdio.h>
#include <string.h>
#include <pthread.h>
struct Spin { pthread_spinlock_t dummy1, s1, dummy2, s2, dummy3; };
int main (int argc, char *argv[]) {
  struct Spin spin;
  memset (&spin, 0xffffffff, sizeof (spin));
  if (pthread_spin_init (&spin.s1, 0) == 0 && pthread_spin_init (&spin.s2, 0) == 0 &&
      sizeof (pthread_spinlock_t) == 4 && spin.s1 == spin.s2)
    { // # sizeof==4 and location-independence are current implementation assumption
      FILE *f = fopen ("conftest_spinlock_initializer.txt", "w");
      fprintf (f, "/*{*/ 0x%04x, /*}*/\n", *(int*) &spin.s1);
      fclose (f);
    }
  return 0;
}
endef

