# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == conftest_header_symbol ==
conftest_header_symbol =  { { : \
  && echo '\#include <$1>' \
  && echo 'int main() {' \
  && echo '\#ifndef $2' \
  && echo '($2);' \
  && echo '\#endif' \
  && echo 'return 0; }' ; } > "conftest_header_symbol-$1.c" \
  && { V=$$(echo '$2' | sed -e 's/[^a-z0-9A-Z]/_/g' -e 's/\(.*\)/\U\1/') \
       && $(CXX) -fpermissive "conftest_header_symbol-$1.c" -o "conftest_header_symbol-$1" 2>/dev/null \
       && echo "\#define HAVE_$$V			1 // $1" \
       || echo "\#undef  HAVE_$$V" ; } \
  && rm -f "conftest_header_symbol-$1.c" "conftest_header_symbol-$1" ; }

# == conftest_lib & conftest_require_lib ==
# $(call conftest_lib, header, symbol, lib) -> $CONFTEST
conftest_lib = { { echo '\#include <$(strip $1)>' \
                && echo 'int main() { return 0 == (int) (long) (void*) &($2); }' ; } > "conftest_lib-$$$$.cc" \
		&& { CONFTEST_LOG=$$($(CXX) -fpermissive "conftest_lib-$$$$.cc" -o "conftest_lib-$$$$" $(LDFLAGS) $3 2>&1) \
		     && CONFTEST=true || CONFTEST=false ; } \
		&& rm -f "conftest_lib-$$$$.cc" "conftest_lib-$$$$" ; }
conftest_lib.makefile ::= $(lastword $(MAKEFILE_LIST))
# $(call conftest_require_lib, header, symbol, lib) -> errors if $CONFTEST != true
conftest_require_lib = { $(call conftest_lib,$1,$2,$3) && $$CONFTEST \
	|| { echo "$$CONFTEST_LOG" | sed 's/^/> /'; \
	     echo '$(conftest_lib.makefile):' "ERROR: Failed to link with '$3' against symbol:" '$2'; \
	     exit 7; } >&2 ; }

# == config-checks.requirements ==
config-checks.require.pkgconfig ::= $(strip	\
	alsa			>= 1.0.5	\
	flac			>= 1.2.1	\
	fluidsynth		>= 1.0.6	\
	ogg			>= 1.2.2	\
	vorbis			>= 1.3.2	\
	vorbisenc		>= 1.3.2	\
	vorbisfile		>= 1.3.2	\
	glib-2.0        	>= 2.32.3	\
	gmodule-no-export-2.0	>= 2.32.3	\
	gthread-2.0		>= 2.32.3	\
	gobject-2.0		>= 2.32.3	\
	libart-2.0		>= 2.3.8	\
        pangoft2        	>= 1.30.0	\
	gtk+-2.0		>= 2.12.12	\
	libgnomecanvas-2.0	>= 2.4.0	\
)
# mad.pc exists in Debian only:	mad >= 0.14.2
# VORBISFILE_BAD_SEEK indicates pcm_seek bug near EOF for small files in vorbisfile <= 1.3.4

# == config-cache.mk ==
GLIB_PACKAGES    ::= glib-2.0 gobject-2.0 gmodule-no-export-2.0
# used for GLIB_CFLAGS and GLIB_LIBS
BSEDEPS_PACKAGES ::= fluidsynth vorbisenc vorbisfile vorbis ogg flac zlib $(GLIB_PACKAGES) # mad
# used for BSEDEPS_CFLAGS BSEDEPS_LIBS
-include $>/config-cache.mk
$>/config-cache.mk: config-checks.mk version.sh $(GITCOMMITDEPS) | $>/./
	$(QECHO) CHECK    Configuration dependencies...
	$Q $(PKG_CONFIG) --exists --print-errors '$(config-checks.require.pkgconfig)'
	$(QGEN)
	$Q echo '# make $@'					> $@.tmp
	$Q V=$$(./version.sh -l) \
	  && echo "BUILDID ::= $$V"				>>$@.tmp
	$Q V=$$(./version.sh -d) \
	  && echo "VERSION_DATE ::= $$V"			>>$@.tmp
	$Q VERSION_SHORT=$$(./version.sh -s) \
	  && echo "VERSION_SHORT ::= $$VERSION_SHORT"		>>$@.tmp \
	  && echo "$$VERSION_SHORT" | sed 's/[^0-9]/ /g'	> $@.tmpv \
	  && read MAJOR MINOR MICRO REST			< $@.tmpv \
	  && rm -f						  $@.tmpv \
	  && echo "VERSION_MAJOR ::= $$MAJOR"			>>$@.tmp \
	  && echo "VERSION_MINOR ::= $$MINOR"			>>$@.tmp \
	  && echo "VERSION_MICRO ::= $$MICRO"			>>$@.tmp \
	  && echo "BSE_GETTEXT_DOMAIN ::=" \
		"beast-$$MAJOR.$$MINOR.$$MICRO" >>$@.tmp
	$Q GLIB_CFLAGS=$$(pkg-config --cflags $(GLIB_PACKAGES)) \
	  && echo "GLIB_CFLAGS ::= $$GLIB_CFLAGS"		>>$@.tmp
	$Q GLIB_LIBS=$$(pkg-config --libs $(GLIB_PACKAGES)) \
	  && echo "GLIB_LIBS ::= $$GLIB_LIBS"			>>$@.tmp
	$Q BSEDEPS_CFLAGS=$$(pkg-config --cflags $(BSEDEPS_PACKAGES)) \
	  && echo "BSEDEPS_CFLAGS ::= $$BSEDEPS_CFLAGS"		>>$@.tmp
	$Q BSEDEPS_LIBS=$$(pkg-config --libs $(BSEDEPS_PACKAGES)) \
	  && echo "BSEDEPS_LIBS ::= $$BSEDEPS_LIBS"		>>$@.tmp
	$Q MAD_LIBS='-lmad' \
	  && echo "MAD_LIBS ::= $$MAD_LIBS"			>>$@.tmp \
	  && echo 'BSEDEPS_LIBS += $$(MAD_LIBS)'		>>$@.tmp \
	  && $(call conftest_require_lib, mad.h, mad_stream_errorstr, $$MAD_LIBS)
	$Q $(PKG_CONFIG) --exists 'vorbisfile <= 1.3.4' && BAD_SEEK=1 || BAD_SEEK=0 \
	  && echo "VORBISFILE_BAD_SEEK ::= $$BAD_SEEK"		>>$@.tmp
	$Q echo 'config-stamps ::= $$>/config-stamps.sha256'	>>$@.tmp \
	  && OLDSUM=$$(cat "$>/config-stamps.sha256" 2>/dev/null || :) \
	  && TMPSUM=$$(sha256sum < $@.tmp) && CFGSUM="$${TMPSUM%-}$(@F)" \
	  && (test "$$OLDSUM" = "$$CFGSUM" \
	      &&   echo '  KEEP     $>/config-stamps.sha256' \
	      || { echo '  UPDATE   $>/config-stamps.sha256' \
		   && echo -e 'MAKECMDGOALS ?= all\n$${MAKECMDGOALS}: ;' \
			'@$$(MAKE) -C '\''${PWD}'\'' $${MAKECMDGOALS}' > $>/GNUmakefile \
		   && echo "$$CFGSUM" > $>/config-stamps.sha256 \
		   ; } )
	$Q mv $>/config-cache.mk $>/config-cache.old 2>/dev/null || true
	$Q mv $@.tmp $@
$>/config-stamps.sha256: $>/config-cache.mk
CLEANFILES += $>/config-stamps.sha256 $>/config-cache.mk $>/config-cache.old
# About config-stamps.sha256: For a variety of reasons, config-cache.mk may be
# often regenerated. To efficiently detect changes in the build configuration,
# use $(config-stamps) as dependency.
# Note, consider variables defined in config-cache.mk as stale within the recipe
# for config-cache.mk. I.e. use "$$MAJOR" instead of "$(VERSION_MINOR)" to avoid
# 2-phase regeneration of config-cache.mk that trips up config-stamps.sha256.
