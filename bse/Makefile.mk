# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/bse/*.d)
bse/cleandirs ::= $(wildcard $>/bse/ $>/lib/)
CLEANDIRS      += $(bse/cleandirs)
ALL_TARGETS    += bse/all
CHECK_TARGETS  += bse/check

# == bse/ files ==
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
	bse/unicode.cc			\
	bse/weaksym.cc			\
)
bse/include.idls ::= $(strip		\
	bse/bse.idl			\
	bse/bsebasics.idl		\
	bse/bsebusmodule.idl		\
	bse/bsecxxbase.idl		\
	bse/bsecxxmodule.idl		\
	$>/bse/bsehack.idl		\
)
bse/libbse.deps     ::= $(strip		\
	$>/bse/bseapi_handles.hh	\
	$>/bse/bseapi_interfaces.hh	\
	$>/bse/bsebasics.genidl.hh	\
	$>/bse/bsebusmodule.genidl.hh	\
	$>/bse/bsegentypes.h		\
	$>/bse/sysconfig.h		\
)
bse/libbse.cc.deps  ::= $(strip		\
	$>/bse/bseenum_arrays.cc	\
	$>/bse/bseenum_list.cc		\
	$>/bse/bsegenbasics.cc		\
	$>/bse/bsegentype_array.cc	\
	$>/bse/bsegentypes.cc		\
	$>/bse/gslfft.cc		\
	$>/bse/zres.cc			\
)
bse/bseapi.idl.outputs		::= $>/bse/bseapi_interfaces.hh $>/bse/bseapi_interfaces.cc $>/bse/bseapi_handles.hh $>/bse/bseapi_handles.cc
# BROKEN ::= bse/bsemididevice-oss.hh bse/bsemididevice-oss.cc  bse/bsepcmdevice-oss.hh bse/bsepcmdevice-oss.cc

# == libbse.so defs ==
lib/libbse.so			::= $>/lib/libbse-$(VERSION_MAJOR).so.$(VERSION_MINOR).$(VERSION_MICRO)
bse/libbse.objects		::= $(call BUILDDIR_O, $(bse/libbse.sources))
bse/include.headerdir		::= $(pkglibdir)/include/bse
bse/include.headers		::= $(bse/libbse.headers) $(bse/libbse.deps) $(bse/include.idls)
bse/all: $(lib/libbse.so)

# == bseprocidl defs ==
bse/bseprocidl			::= $>/bse/bseprocidl
bse/bseprocidl.sources		::= bse/bseprocidl.cc
bse/bseprocidl.objects		::= $(call BUILDDIR_O, $(bse/bseprocidl.sources))
bse/bseprocidl.objects.FLAGS	  = -O0	# compile fast
bse/all: $(bse/bseprocidl)

# == integrity defs ==
bse/integrity		   ::= $>/bse/integrity
bse/integrity.sources	   ::= bse/integrity.cc
bse/integrity.objects	   ::= $(call BUILDDIR_O, $(bse/integrity.sources))
bse/integrity.objects.FLAGS  = -O0	# compile fast
bse/all: $(bse/integrity)

# == subdirs ==
include bse/icons/Makefile.mk

# == libbse.so rules ==
$(bse/libbse.objects): $(bse/libbse.deps) $(bse/libbse.cc.deps) $(bse/icons/c.csources)
$(bse/libbse.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(bse/libbse.objects): EXTRA_DEFS ::= -DBSE_COMPILATION
$(lib/libbse.so).LDFLAGS ::= -Wl,--version-script=bse/ldscript.map
$(call BUILD_SHARED_LIB_XDBG, \
	$(lib/libbse.so), \
	$(bse/libbse.objects), \
	bse/ldscript.map | $>/lib/, \
	$(BSEDEPS_LIBS))
$(call INSTALL_DATA_RULE,			\
	bse/headers,				\
	$(DESTDIR)$(bse/include.headerdir),	\
	$(bse/include.headers) $(bse/libbse.deps))
$(call $(if $(filter release, $(MODE)), INSTALL_BIN_RULE, INSTALL_BIN_RULE_XDBG), \
	lib/libbse, $(DESTDIR)$(pkglibdir)/lib, $(lib/libbse.so))

# == bseapi.idl rules ==
$(call MULTIOUTPUT, $(bse/bseapi.idl.outputs)): bse/bseapi.idl	bse/bseapi-inserts.hh $(aidacc/aidacc) bse/AuxTypes.py	| $>/bse/
	$(QECHO) GEN $(bse/bseapi.idl.outputs) # aidacc generates %_interfaces.{hh|cc} %_handles.{hh|cc} from %.idl, and the real MULTIOUTPUT target name looks wierd
	$Q $(aidacc/aidacc) -x CxxStub -x bse/AuxTypes.py -G strip-path=$(abspath .)/ --insertions bse/bseapi-inserts.hh -o $>/bse $<
	$Q sed '1i#define _(x) x' -i $>/bse/bseapi_interfaces.cc && sed '1i#undef _' -i $>/bse/bseapi_interfaces.cc

# == sfidl rules ==
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

# == gslfft.cc ==
$>/bse/gslfft.cc: bse/gsl-fftconf.sh bse/gsl-fftgen.pl	| $>/bse/
	$(QGEN)
	$Q bse/gsl-fftconf.sh \
		'$(PERL) bse/gsl-fftgen.pl $(if $(findstring 1, $V),, --no-verbose)' \
		'"bse/gslfft.hh"'						> $@.tmp
	$Q mv $@.tmp $@

# == zres.cc ==
$>/bse/zres.cc: res/resfiles.list misc/packres.py	| $>/bse/	# $(res_resfiles_list) is set to the contents of res/resfiles.list
	$(QGEN)
	$Q misc/packres.py -s 'res/' $(res_resfiles_list:%=res/%) > $@.tmp
	$Q mv $@.tmp $@

# == buildid.cc ==
$>/bse/buildid.cc: $(config-stamps) $(GITCOMMITDEPS)	| $>/bse/
	$(QGEN)
	$Q echo '// make $@'							> $@.tmp \
	  && V="$(VERSION_LONG)" \
	  && echo "static const char *static_bse_version_buildid = \"$$V\";"	>>$@.tmp \
	  && V="$(VERSION_DATE)" \
	  && echo "static const char *static_bse_version_date = \"$$V\";"	>>$@.tmp
	$Q mv $@.tmp $@
$>/bse/weaksym.o: $>/bse/buildid.cc
# Avoid massive rebuilds when buildid.cc changes, so we avoid adding it to
# bse/libbse.deps. Only bse/weaksym.cc actually includes buildid.cc.

# == bse/sysconfig.h ==
$>/bse/sysconfig.h: $(config-stamps) | $>/bse/ # bse/Makefile.mk
	$(QGEN)
	$Q echo '// make $@'							> $@.tmp
	$Q echo '#define BSE_MAJOR_VERSION		($(VERSION_MAJOR))'	>>$@.tmp
	$Q echo '#define BSE_MINOR_VERSION		($(VERSION_MINOR))'	>>$@.tmp
	$Q echo '#define BSE_MICRO_VERSION		($(VERSION_MICRO))'	>>$@.tmp
	$Q echo '#define BSE_VERSION_STRING		"$(VERSION_SHORT)"'	>>$@.tmp
	$Q echo '#define BSE_GETTEXT_DOMAIN		"$(BSE_GETTEXT_DOMAIN)"'>>$@.tmp
	$Q echo '#define BSE_VORBISFILE_BAD_SEEK 	$(VORBISFILE_BAD_SEEK)'	>>$@.tmp
	$Q : $(file > $>/bse/conftest_spinlock.c, $(bse/conftest_spinlock.c)) \
	&& $(CC) -Wall $>/bse/conftest_spinlock.c -pthread -o $>/bse/conftest_spinlock \
	&& (cd $> && ./bse/conftest_spinlock) \
	&& echo '#define BSE_SPINLOCK_INITIALIZER' "	$$(cat $>/bse/conftest_spinlock.txt)"	>>$@.tmp \
	&& rm $>/bse/conftest_spinlock.c $>/bse/conftest_spinlock $>/bse/conftest_spinlock.txt
	$Q mv $@.tmp $@
# bse/conftest_spinlock.c
define bse/conftest_spinlock.c
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
      FILE *f = fopen ("bse/conftest_spinlock.txt", "w");
      fprintf (f, "/*{*/ 0x%04x, /*}*/\n", *(int*) &spin.s1);
      fclose (f);
    }
  return 0;
}
endef

# == bseprocidl rules ==
$(bse/bseprocidl.objects):	$(bse/libbse.deps)
$(bse/bseprocidl.objects):	EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_PROGRAM, \
	$(bse/bseprocidl), \
	$(bse/bseprocidl.objects), \
	$(lib/libbse.so), \
	-lbse-$(VERSION_MAJOR) $(GLIB_LIBS), ../lib)

# == bsehack.idl ==
# currently generated from BSE introspection data, needed to build the old IDL bindings for beast-gtk
$>/bse/bsehack.idl: bse/bse.idl bse/bsebasics.idl bse/bsecxxbase.idl bse/bsecxxmodule.idl | $>/bse/bseprocidl
	$(QGEN)
	$Q echo '// make $@'							> $@.tmp1
	$Q grep -v 'include.*bsehack.idl' bse/bse.idl | \
		$(sfi/sfidl) $(sfi/sfidl.includes) -Ibse --list-types -		> $@.tmp2
	$Q $>/bse/bseprocidl $@.tmp2 --g-fatal-warnings				>>$@.tmp1
	$Q mv $@.tmp1 $@ && rm -f $@.tmp2

# == integrity rules ==
$(bse/integrity.objects):     $(bse/libbse.deps) | $>/bse/
$(bse/integrity.objects):     EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_TEST, \
	$(bse/integrity), \
	$(bse/integrity.objects), \
	$(lib/libbse.so), \
	-lbse-$(VERSION_MAJOR), \
	../lib)

# == bse-check-assertions ==
$>/bse/t279-assertions-test: FORCE	$(bse/integrity)
	$(QECHO) RUN $@
	$Q $(bse/integrity) --return_unless1 || $(QDIE) --return_unless1 failed
	$Q $(bse/integrity) --assert_return1 || $(QDIE) --assert_return1 failed
	$Q (trap ':' SIGTRAP && $(bse/integrity) --return_unless0) $(QSTDERR) ; test "$$?" -eq 7 || $(QDIE) --return_unless0 failed
	$Q (trap ':' SIGTRAP && $(bse/integrity) --assert_return0) $(QSTDERR) ; test "$$?"  != 0 || $(QDIE) --assert_return0 failed
	$Q (trap ':' SIGTRAP && $(bse/integrity) --assert_return_unreached) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --assert_return_unreached failed
	$Q (trap ':' SIGTRAP && $(bse/integrity) --fatal_error) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --fatal_error failed
	$Q $(bse/integrity) --backtrace	2> $@.tmp && \
		grep -qi 'Backtrace'		   $@.tmp && \
		grep -qi 'in.*my_compare_func'	   $@.tmp || $(QDIE) --backtrace failed
	$Q rm $@.tmp
	@echo "  PASS    " $@
bse/check: $>/bse/t279-assertions-test

# == bse/clean ==
bse/clean: FORCE
	rm -f -r $(bse/cleandirs)
