# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/bse/*.d)


# == libbse.so ==
bse/libbse.sources  ::= $(strip		\
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
bse/libbse.sources += bse/gslfft.cc
bse.so              ::= bse-$(VERSION_MAJOR).so
libbse.soname       ::= lib$(bse.so).$(VERSION_MINOR)
bse/libbse.sofile   ::= $>/bse/$(libbse.soname).$(VERSION_MICRO)
bse/libbse.solinks  ::= $>/bse/$(libbse.soname) $>/bse/lib$(bse.so)
ALL_TARGETS	     += $(bse/libbse.sofile) $(bse/libbse.solinks)
bse/libbse.deps     ::= $>/bse/sysconfig.h
bse/libbse.objects ::= $(sort $(bse/libbse.sources:%.cc=$>/%.o))
$(bse/libbse.objects): $(bse/libbse.deps) | $>/bse/
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
CLEANDIRS += $(wildcard $>/bse/)


# == bsetool ==
bse/bsetool         ::= $>/bse/bsetool
ALL_TARGETS	     += $(bse/bsetool)
bse/bsetool.sources ::= bse/bsetool.cc
bse/bsetool.objects ::= $(sort $(bse/bsetool.sources:%.cc=$>/%.o))
bse/bsetool.deps    ::= $>/bse/sysconfig.h
$(bse/bsetool.objects): $(bse/bsetool.deps)
$(bse/bsetool.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
# BIN linking
$(bse/bsetool): $(bse/bsetool.objects) $(bse/libbse.solinks) $(MAKEFILE_LIST)
	$(QECHO) LD $@
	$Q $(CXX) -fPIC -o $@ $(bse/bsetool.objects) -Wl,--no-undefined \
	-Wl,-rpath='$$ORIGIN/../bse:$$ORIGIN/' -Wl,-L$>/bse/ -lbse-$(VERSION_MAJOR) \
	$(GLIB_LIBS) `pkg-config --libs gobject-2.0 vorbisfile vorbisenc vorbis ogg mad fluidsynth gmodule-no-export-2.0 flac` -lz
# CUSTOMIZATIONS: bse/bsetool.cc.FLAGS = -O2   ||   $>/bse/bsetool.o.FLAGS = -O3    ||    $>/bse/bsetool.o: EXTRA_FLAGS = -O1


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

