# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/bse/*.d)
bse/cleandirs ::= $(wildcard $>/bse/ $>/lib/)
CLEANDIRS      += $(bse/cleandirs)
ALL_TARGETS    += bse/all
CHECK_TARGETS  += bse/check

# == BeastSoundEngine definitions ==
lib/BeastSoundEngine		::= $>/lib/BeastSoundEngine-$(VERSION_M.M.M)
bse/BeastSoundEngine.deps	::= $>/bse/bseapi_jsonipc.cc
bse/BeastSoundEngine.headers	::= bse/beast-sound-engine.hh
bse/BeastSoundEngine.sources	::= bse/beast-sound-engine.cc bse/jsonipcstubs.cc
bse/BeastSoundEngine.gensources ::= $>/bse/bse_jsonipc_stub1.cc $>/bse/bse_jsonipc_stub2.cc $>/bse/bse_jsonipc_stub3.cc $>/bse/bse_jsonipc_stub4.cc
bse/BeastSoundEngine.objects	::= $(call BUILDDIR_O, $(bse/BeastSoundEngine.sources)) $(bse/BeastSoundEngine.gensources:.cc=.o)
bse/all: $(lib/BeastSoundEngine)

# == libbsejack.so definitions ==
bse/libbsejack.sources		::= bse/driver-jack.cc
lib/libbsejack.so		::= $>/lib/libbse-jack-$(VERSION_M.M.M).so
bse/libbsejack.objects		::= $(call BUILDDIR_O, $(bse/libbsejack.sources))
ifneq ('','$(BSE_JACK_LIBS)')
  bse/all: $(lib/libbsejack.so)
endif

# == libbse.so definitions ==
bse/libbse.exclude      ::= $(bse/BeastSoundEngine.sources) $(bse/libbsejack.sources)
bse/libbse.csources     ::= $(sort $(filter-out %.inc.c, $(wildcard bse/*.c)))
bse/libbse.sources      ::= $(sort $(filter-out $(bse/libbse.exclude) %.inc.cc, $(bse/libbse.csources) $(wildcard bse/*.cc)))
bse/libbse.headers      ::= $(sort $(filter-out $(bse/BeastSoundEngine.headers) %.inc.hh, $(wildcard bse/*.hh)))
bse/libbse.bseonly.headers = $(filter bse/bse%, $(bse/libbse.headers))
bse/include.idls ::= $(strip		\
	bse/bsebasics.idl		\
	bse/bsebusmodule.idl		\
	bse/bsecxxbase.idl		\
	bse/bsecxxmodule.idl		\
)
bse/libbse.deps     ::= $(strip		\
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
bse/bseapi.idl.outputs		::= $>/bse/bseapi_interfaces.hh $>/bse/bseapi_interfaces.cc
lib/libbse.so			::= $>/lib/libbse-$(VERSION_MAJOR).so.$(VERSION_MINOR).$(VERSION_MICRO)
bse/libbse.objects		::= $(call BUILDDIR_O, $(bse/libbse.sources))
bse/libbse.objects		 += $(devices/libbse.objects)
bse/include.headerdir		::= $(pkglibdir)/include/bse
bse/include.headers		::= $(bse/libbse.headers) $(bse/libbse.deps) $(bse/include.idls)
bse/all: $(lib/libbse.so)

# == external/minizip ==
$>/external/minizip/mz_zip.h:			| $>/external/
	$(QECHO) FETCH "minizip-2.9.0"
	$Q cd $>/external/ \
	     $(call AND_DOWNLOAD_SHAURL, \
		0c68fc9653203ca59a4b83598c89a86156c9142e08edbea9ae2430ee1e31babb, \
			https://github.com/nmoinvaz/minizip/archive/2.9.0.zip)
	$(QGEN)
	$Q cd $>/external/ && rm -rf minizip-2.9.0/ && unzip -q 2.9.0.zip
	$Q rm $>/external/2.9.0.zip && rm -rf $>/external/minizip/
	$Q mv $>/external/minizip-2.9.0/ $>/external/minizip/
CLEANDIRS += $>/external/minizip
$(bse/libbse.objects): $>/external/minizip/mz_zip.h

# == subdirs ==
include bse/icons/Makefile.mk

# == libbse.so rules ==
$(bse/libbse.objects): $(bse/libbse.deps) $(bse/libbse.cc.deps) $(bse/icons/c.csources)
$(bse/libbse.objects): EXTRA_INCLUDES ::= -I$> $(BSEDEPS_CFLAGS)
$(bse/libbse.objects): EXTRA_DEFS ::= -DBSE_COMPILATION
$(lib/libbse.so).LDFLAGS ::= -Wl,--version-script=bse/ldscript.map
$(call BUILD_SHARED_LIB_XDBG, \
	$(lib/libbse.so), \
	$(bse/libbse.objects), \
	bse/ldscript.map | $>/lib/, \
	$(BSEDEPS_LIBS) $(ALSA_LIBS) -lstdc++fs)
$(call INSTALL_DATA_RULE,			\
	bse/headers,				\
	$(DESTDIR)$(bse/include.headerdir),	\
	$(bse/include.headers) $(bse/libbse.deps))
$(call $(if $(filter production, $(MODE)), INSTALL_BIN_RULE, INSTALL_BIN_RULE_XDBG), \
	lib/libbse, $(DESTDIR)$(pkglibdir)/lib, $(lib/libbse.so))
$(bse/libbse.objects): | $>/bse/check-stray-sources
$>/bse/check-stray-sources: $(bse/libbse.headers) $(bse/libbse.sources)
	$(QECHO) CHECK bse/: check stray sources
	$(Q) test -z "$(DOTGIT)" || { \
	  git status -s -- $^ | { grep '^?? .*' && \
		{ echo 'bse/: error: untracked source files present'; exit 3 ; } || :; \
	  }; }
	$(Q) echo '$^' > $@
ifneq ($(bse/libbse.headers) $(bse/libbse.sources), $(shell cat $>/bse/check-stray-sources 2>/dev/null))
.PHONY: $>/bse/check-stray-sources
endif

# == libbsejack.so rules ==
$(bse/libbsejack.objects): $(bse/libbse.deps) $(bse/libbsejack.cc.deps)
$(bse/libbsejack.objects): EXTRA_INCLUDES ::= -I$> $(BSEDEPS_CFLAGS)
$(bse/libbsejack.objects): EXTRA_DEFS ::= -DBSE_COMPILATION
$(lib/libbsejack.so).LDFLAGS ::= -Wl,--version-script=bse/ldscript.map
ifneq ('','$(BSE_JACK_LIBS)')
$(call BUILD_SHARED_LIB_XDBG, \
	$(lib/libbsejack.so), \
	$(bse/libbsejack.objects), \
	$(lib/libbse.so) bse/ldscript.map | $>/lib/, \
	-lbse-$(VERSION_MAJOR) $(BSE_JACK_LIBS), \
        ../lib)
$(call $(if $(filter production, $(MODE)), INSTALL_BIN_RULE, INSTALL_BIN_RULE_XDBG), \
	lib/libbsejack, $(DESTDIR)$(pkglibdir)/lib, $(lib/libbsejack.so))
endif

# == bseapi.idl rules ==
$(call MULTIOUTPUT, $(bse/bseapi.idl.outputs)): bse/bseapi.idl	bse/bseapi-inserts.inc.hh $(aidacc/aidacc)	| $>/bse/
	$(QECHO) GEN $(bse/bseapi.idl.outputs) # aidacc generates %_interfaces.{hh|cc} from %.idl, and the real MULTIOUTPUT target name looks wierd
	$Q $(aidacc/aidacc) -x CxxStub -G strip-path=$(abspath .)/ --insertions bse/bseapi-inserts.inc.hh -o $>/bse $<
	$Q sed '1i#define _(x) x' -i $>/bse/bseapi_interfaces.cc && sed '1i#undef _' -i $>/bse/bseapi_interfaces.cc

# == sfidl rules ==
$>/bse/bseenum_arrays.cc: $(bse/libbse.bseonly.headers)		| $>/bse/
	$(QGEN)
	$Q $(GLIB_MKENUMS)	--fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
				--vhead "/* @EnumName@\n */\n" \
				--vhead "static G@Type@Value @enum_name@_values[] = { // enum_values\n" \
				--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
				--vtail "  { 0, NULL, NULL }\n};\n" \
				$(bse/libbse.bseonly.headers)			> $@.tmp
	$Q mv $@.tmp $@
$>/bse/bseenum_list.cc: $(bse/libbse.bseonly.headers)		| $>/bse/
	$(QGEN)
	$Q $(GLIB_MKENUMS)	--fprod "\n/* --- @filename@ --- */" \
				--eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &BSE_TYPE_ID (@EnumName@), @enum_name@_values }," \
				$(bse/libbse.bseonly.headers)			> $@.tmp
	$Q mv $@.tmp $@
$>/bse/bsegentypes.h: bse/bsebasics.idl	$(bse/libbse.bseonly.headers) bse/mktypes.pl $(sfi/sfidl) | $>/bse/
	$(QGEN)
	$Q $(GLIB_MKENUMS)	--fprod "\n/* --- @filename@ --- */" \
				--eprod "#define BSE_TYPE_@ENUMSHORT@\t (BSE_TYPE_ID (@EnumName@)) // enum\n" \
				--eprod "extern GType BSE_TYPE_ID (@EnumName@);" \
				$(bse/libbse.bseonly.headers)		> $@.tmp
	$Q $(PERL) bse/mktypes.pl --externs $(bse/libbse.sources)	>>$@.tmp
	$Q $(sfi/sfidl) $(sfi/sfidl.includes)	--core-c --header $<	>>$@.tmp
	$Q mv $@.tmp $@
$>/bse/bsegentypes.cc: $(bse/libbse.bseonly.headers) bse/mktypes.pl | $>/bse/	# $(bse/libbse.sources)
	$(QGEN)
	$Q $(GLIB_MKENUMS)	--eprod "\nGType BSE_TYPE_ID (@EnumName@) = 0;" \
				$(bse/libbse.bseonly.headers)				> $@.tmp
	$Q $(PERL) bse/mktypes.pl --interns --export-proto $(bse/libbse.sources)	>>$@.tmp
	$Q mv $@.tmp $@
$>/bse/bsegentype_array.cc: $(bse/libbse.bseonly.headers) bse/mktypes.pl | $>/bse/  # $(bse/libbse.sources)
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
	$Q misc/packres.py -z -s 'res/' $(res_resfiles_list:%=res/%) > $@.tmp
	$Q mv $@.tmp $@

# == bse/sysconfig.h ==
$>/bse/sysconfig.h: $(config-stamps) $>/bse/sysconfig-1.h $>/bse/sysconfig-2.h | $>/bse/ # bse/Makefile.mk
	$(QGEN)
	$Q echo '// make $@'							> $@.tmp
	$Q echo '#define BSE_MAJOR_VERSION		($(VERSION_MAJOR))'	>>$@.tmp
	$Q echo '#define BSE_MINOR_VERSION		($(VERSION_MINOR))'	>>$@.tmp
	$Q echo '#define BSE_MICRO_VERSION		($(VERSION_MICRO))'	>>$@.tmp
	$Q echo '#define BSE_VERSION_STRING		"$(VERSION_SHORT)"'	>>$@.tmp
	$Q echo '#define BSE_GETTEXT_DOMAIN		"$(BSE_GETTEXT_DOMAIN)"'>>$@.tmp
	$Q echo '#define BSE_VORBISFILE_BAD_SEEK 	$(VORBISFILE_BAD_SEEK)'	>>$@.tmp
	$Q cat $>/bse/sysconfig-1.h $>/bse/sysconfig-2.h			>>$@.tmp
	$Q mv $@.tmp $@

# == bse/sysconfig-1 ==
$>/bse/sysconfig-1.h: $(config-stamps) bse/Makefile.mk	| $>/bse/
	$(QGEN)
	$Q echo '#include <time.h>'				> $@-timegm.c \
	&& echo 'void main() { struct tm t; timegm (&t); }'	>>$@-timegm.c \
	&& $(CC) $@-timegm.c -o $@-timegm 2>/dev/null \
	&& echo -e '#define BSE_HAVE_TIMEGM \t\t 1'		> $@ \
	|| echo '// #undef BSE_HAVE_TIMEGM'			> $@ \
	&& rm -f $@-timegm.c $@-timegm

# == bse/sysconfig-2 ==
$>/bse/sysconfig-2.h: $(config-stamps) bse/Makefile.mk	| $>/bse/
	$(QGEN)
	$Q : $(file > $>/bse/conftest_spinlock.c, $(bse/conftest_spinlock.c)) \
	&& $(CC) -Wall $>/bse/conftest_spinlock.c -pthread -o $>/bse/conftest_spinlock \
	&& (cd $> && ./bse/conftest_spinlock) \
	&& echo '#define BSE_SPINLOCK_INITIALIZER' "	$$(cat $>/bse/conftest_spinlock.txt)" > $@ \
	&& rm $>/bse/conftest_spinlock.c $>/bse/conftest_spinlock $>/bse/conftest_spinlock.txt
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

# == bseapi_jsonipc.cc ==
$>/bse/bseapi_jsonipc.cc: bse/bseapi.idl aidacc/JsonipcStub.py $(aidacc/aidacc)	| $>/bse/
	$(QECHO) AIDACC $@
	$Q $(aidacc/aidacc) -x aidacc/JsonipcStub.py $< -o $@.tmp -G nblocks=4 -G strip-path=$(abspath $>)/
	$Q rm -f $@ && chmod -w $@.tmp
	$Q mv $@.tmp $@
$>/bse/bse_jsonipc_stub1.cc: $>/bse/bseapi_jsonipc.cc
	$(QGEN)
	$Q echo '#include "bse/beast-sound-engine.hh"'			> $@.tmp
	$Q echo '#define BLOCK_TYPES 1'					>>$@.tmp
	$Q echo '#include "bseapi_jsonipc.cc"'				>>$@.tmp
	$Q echo 'void bse_jsonipc_stub1 () { Bse_jsonipc_stub(); }'	>>$@.tmp
	$Q mv $@.tmp $@
$>/bse/bse_jsonipc_stub2.cc: $>/bse/bseapi_jsonipc.cc
	$(QGEN)
	$Q echo '#include "bse/beast-sound-engine.hh"'			> $@.tmp
	$Q echo '#define BLOCK_TYPES 2'					>>$@.tmp
	$Q echo '#include "bseapi_jsonipc.cc"'				>>$@.tmp
	$Q echo 'void bse_jsonipc_stub2 () { Bse_jsonipc_stub(); }'	>>$@.tmp
	$Q mv $@.tmp $@
$>/bse/bse_jsonipc_stub3.cc: $>/bse/bseapi_jsonipc.cc
	$(QGEN)
	$Q echo '#include "bse/beast-sound-engine.hh"'			> $@.tmp
	$Q echo '#define BLOCK_TYPES 3'					>>$@.tmp
	$Q echo '#include "bseapi_jsonipc.cc"'				>>$@.tmp
	$Q echo 'void bse_jsonipc_stub3 () { Bse_jsonipc_stub(); }'	>>$@.tmp
	$Q mv $@.tmp $@
$>/bse/bse_jsonipc_stub4.cc: $>/bse/bseapi_jsonipc.cc
	$(QGEN)
	$Q echo '#include "bse/beast-sound-engine.hh"'			> $@.tmp
	$Q echo '#define BLOCK_TYPES 4'					>>$@.tmp
	$Q echo '#include "bseapi_jsonipc.cc"'				>>$@.tmp
	$Q echo 'void bse_jsonipc_stub4 () { Bse_jsonipc_stub(); }'	>>$@.tmp
	$Q mv $@.tmp $@

# == BeastSoundEngine ==
$(bse/BeastSoundEngine.objects): $(bse/BeastSoundEngine.deps) $(bse/libbse.deps)
$(bse/BeastSoundEngine.objects): EXTRA_INCLUDES ::= -I$> -Iexternal/ $(GLIB_CFLAGS)
$(bse/BeastSoundEngine.objects): EXTRA_FLAGS ::= -Wno-sign-promo
$(call BUILD_PROGRAM, \
	$(lib/BeastSoundEngine), \
	$(bse/BeastSoundEngine.objects), \
	$(lib/libbse.so), \
	-lbse-$(VERSION_MAJOR) $(BOOST_SYSTEM_LIBS) $(BSEDEPS_LIBS), \
	../lib)
$(call INSTALL_BIN_RULE, $(basename $(lib/BeastSoundEngine)), $(DESTDIR)$(pkglibdir)/lib, $(lib/BeastSoundEngine))

# == bse/clean ==
bse/clean: FORCE
	rm -f -r $(bse/cleandirs)
