# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == System Utils ==
PERL			?= perl
PYTHON2 		?= python2.7
YAPPS			?= $(PYTHON2) $(abspath yapps2_deb/yapps2.py)
PKG_CONFIG		?= pkg-config
GLIB_MKENUMS		?= glib-mkenums
GLIB_GENMARSHAL 	?= glib-genmarshal
GDK_PIXBUF_CSOURCE	?= gdk-pixbuf-csource
IMAGEMAGICK_CONVERT	?= convert
PANDOC			?= pandoc
CP			?= cp --reflink=auto
.config.defaults	+= CP PERL PYTHON2 YAPPS PKG_CONFIG GLIB_MKENUMS GLIB_GENMARSHAL GDK_PIXBUF_CSOURCE PANDOC IMAGEMAGICK_CONVERT

INSTALL 		:= /usr/bin/install -c
INSTALL_DATA 		:= $(INSTALL) -m 644
MSGFMT			:= /usr/bin/msgfmt
MSGMERGE		:= /usr/bin/msgmerge
XGETTEXT		:= /usr/bin/xgettext
UPDATE_DESKTOP_DATABASE	:= /usr/bin/update-desktop-database
UPDATE_MIME_DATABASE	:= /usr/bin/update-mime-database

# Check for fast linker
ifeq ($(MODE),release)
useld_fast		::= # keep default linker
useld_fast+vs		::= # keep default linker
# Keep the default linker for release mode, as usually, bfd optimizes better than lld,
# and lld optimizes better than gold in terms of resulting binary size.
else
# Generally, ld.gold is faster than ld.bfd, and ld.lld is often faster than ld.gold for linking
# executables. But ld.lld 6.0.0 has a bug that causes deletion of [abi:cxx11] symbols in
# combination with certain --version-script uses: https://bugs.llvm.org/show_bug.cgi?id=36777
# So avoid ld.lld <= 6 if --version-script is used.
useld_gold		!= ld.gold --version 2>&1 | grep -q '^GNU gold' && echo '-fuse-ld=gold'
useld_lld		!= ld.lld --version 2>&1 | grep -q '^LLD ' && echo '-fuse-ld=lld'
useld_fast		:= $(or $(useld_lld), $(useld_gold))
useld_lld+vs		!= ld.lld --version 2>&1 | grep -v '^LLD [0123456]\.' | grep -q '^LLD ' && echo '-fuse-ld=lld'
useld_fast+vs		:= $(or $(useld_lld+vs), $(useld_gold))
endif

# == conftest_lib & conftest_require_lib ==
# $(call conftest_lib, header, symbol, lib) -> $CONFTEST
conftest_lib = { { echo '\#include <$(strip $1)>' \
                && echo 'int main() { return 0 == (int) (long) (void*) &($2); }' ; } > "$>/conftest_lib-$$$$.cc" \
		&& { CONFTEST_LOG=$$($(CXX) -fpermissive "$>/conftest_lib-$$$$.cc" -o "$>/conftest_lib-$$$$" $(LDFLAGS) $3 2>&1) \
		     && CONFTEST=true || CONFTEST=false ; } \
		&& rm -f "$>/conftest_lib-$$$$.cc" "$>/conftest_lib-$$$$" ; }
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
GTK_PACKAGES     ::= gtk+-2.0 libgnomecanvas-2.0 zlib
# used for GLIB_CFLAGS and GLIB_LIBS
BSEDEPS_PACKAGES ::= fluidsynth vorbisenc vorbisfile vorbis ogg flac zlib $(GLIB_PACKAGES) # mad
# used for BSEDEPS_CFLAGS BSEDEPS_LIBS
-include $>/config-cache.mk
$>/config-cache.mk: config-checks.mk version.sh $(GITCOMMITDEPS) | $>/./
	$(QECHO) CHECK    Configuration dependencies...
	$Q $(PKG_CONFIG) --exists --print-errors '$(config-checks.require.pkgconfig)'
	$Q $(IMAGEMAGICK_CONVERT) --version 2>&1 | grep -q 'Version:.*\bImageMagick\b' \
	  || { echo "$@: failed to detect ImageMagick convert: $(IMAGEMAGICK_CONVERT)" >&2 ; false ; }
	$Q $(PANDOC) --version 2>&1 | grep -q '\bpandoc\b' \
	  || { echo "$@: failed to detect pandoc: $(PANDOC)" >&2 ; false ; }
	$(QGEN)
	$Q echo '# make $@'					> $@.tmp
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
	$Q ALSA_LIBS='-lasound' \
	  && echo "ALSA_LIBS ::= $$ALSA_LIBS"			>>$@.tmp \
	  && $(call conftest_require_lib, alsa/asoundlib.h, snd_asoundlib_version, $$ALSA_LIBS)
	$Q MAD_LIBS='-lmad' \
	  && echo "MAD_LIBS ::= $$MAD_LIBS"			>>$@.tmp \
	  && echo 'BSEDEPS_LIBS += $$(MAD_LIBS)'		>>$@.tmp \
	  && $(call conftest_require_lib, mad.h, mad_stream_errorstr, $$MAD_LIBS)
	$Q $(PKG_CONFIG) --exists 'vorbisfile <= 1.3.4' && BAD_SEEK=1 || BAD_SEEK=0 \
	  && echo "VORBISFILE_BAD_SEEK ::= $$BAD_SEEK"		>>$@.tmp
	$Q GTK_CFLAGS=$$(pkg-config --cflags $(GTK_PACKAGES)) \
	  && echo "GTK_CFLAGS ::= $$GTK_CFLAGS"			>>$@.tmp
	$Q GTK_LIBS=$$(pkg-config --libs $(GTK_PACKAGES)) \
	  && echo "GTK_LIBS ::= $$GTK_LIBS"			>>$@.tmp
	$Q XKB_LIBS='-lX11' L='' \
	  && $(call conftest_lib, X11/XKBlib.h, XkbGetKeyboard, $$XKB_LIBS) \
	  && { ! $$CONFTEST || L="$$XKB_LIBS" ; } \
	  && echo "XKB_LIBS ::= $$L"				>>$@.tmp
	$Q echo 'config-stamps ::= $$>/config-stamps.sha256'	>>$@.tmp \
	  && OLDSUM=$$(cat "$>/config-stamps.sha256" 2>/dev/null || :) \
	  && TMPSUM=$$(sha256sum < $@.tmp) && CFGSUM="$${TMPSUM%-}$(@F)" \
	  && (test "$$OLDSUM" = "$$CFGSUM" \
	      &&   echo '  KEEP     $>/config-stamps.sha256' \
	      || { echo '  UPDATE   $>/config-stamps.sha256' \
		   && echo 'all $${MAKECMDGOALS}:'						> $>/GNUmakefile \
		   && echo '	@ exec $$(MAKE) -C '\''$(abspath .)'\'' $${MAKECMDGOALS}'	>>$>/GNUmakefile \
		   && echo '.PHONY: all'							>>$>/GNUmakefile \
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
