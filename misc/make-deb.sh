#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -e # -x

SCRIPTNAME=`basename $0`
function die  { [ -n "$*" ] && echo "$SCRIPTNAME: $*" >&2; exit 127 ; }

git_clone()
{
  URL="$1" ; REPO="$2" ; COMMIT="$3"
  if test -e ./tmpdeb/$REPO/.git ; then
      (cd ./tmpdeb/$REPO && git checkout master && git pull)
  else
    unset CLONE_REFERENCE
    test -r ../$REPO/.git/index && CLONE_REFERENCE="--dissociate --reference ../$REPO/.git"
    git clone $CLONE_REFERENCE "$URL" ./tmpdeb/$REPO
  fi
  test -z "$COMMIT" || (cd ./tmpdeb/$REPO && git checkout "$COMMIT")
}

build_checked()
{
  REPO="$1" && shift
  CONFIGURE="$1" && shift
  ( cd ./tmpdeb/$REPO
    $CONFIGURE "$@"
    nice -n15 make -j`nproc`
    make check
    make install "DESTDIR=$DESTDIR"
    make installcheck "DESTDIR=$DESTDIR"
  )
}

# create 0755 dirs by default
umask 022

# run in beast/
test -e ./acbeast.m4 || die "failed to detect ./acbeast.m4"

# TODO: add /usr/share/icons/hicolor/48x48/apps/beast.png ./usr/share/icons/hicolor/scalable/apps/beast.svg
# TODO: add /usr/share/icons/hicolor/scalable/mimetypes/application-bse.svg

# build in ./tmpdeb/
BEASTDIR=/opt/$(misc/mkbuildid.sh -p | sed -r 's/^([0-9]+)\.([0-9]+).*/beast-\1-\2/')
DESTDIR=`pwd`/tmpdeb/destdir
RAPICORNPREFIXDIR=$DESTDIR$BEASTDIR
DEBDOCDIR=$DESTDIR$BEASTDIR/doc
DEBIAN=$DESTDIR/DEBIAN
rm -rf $DEBIAN
mkdir -p $DEBIAN

export PATH="$RAPICORNPREFIXDIR/bin:$PATH"
export LD_LIBRARY_PATH="$RAPICORNPREFIXDIR/lib"
export PKG_CONFIG_PATH=$RAPICORNPREFIXDIR/lib/pkgconfig
export PKG_CONFIG_RAPICORN_PREFIX=$RAPICORNPREFIXDIR
export PKG_CONFIG_RAPICORN_17_PREFIX=$RAPICORNPREFIXDIR
export PKG_CONFIG_BSE_BEASTDESTDIR=$DESTDIR
export AIDACC_DESTDIR=$DESTDIR

echo "DISPLAY=$DISPLAY LDFLAGS=$LDFLAGS"
echo "CC=$CC CFLAGS=$CFLAGS"
echo "CXX=$CXX CXXFLAGS=$CXXFLAGS"
echo "BEASTDIR=$BEASTDIR"
echo "DESTDIR=$DESTDIR"
echo "RAPICORNPREFIXDIR=$RAPICORNPREFIXDIR"
echo "PATH=$PATH"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH"
echo "PKG_CONFIG_RAPICORN_PREFIX=$PKG_CONFIG_RAPICORN_PREFIX"
echo "PKG_CONFIG_RAPICORN_17_PREFIX=$PKG_CONFIG_RAPICORN_17_PREFIX"
echo "PKG_CONFIG_BSE_PREFIX=$PKG_CONFIG_BSE_PREFIX"
echo "AIDACC_DESTDIR=$AIDACC_DESTDIR"
echo "DEBIAN=$DEBIAN"

BEASTEXE=$BEASTDIR/bin/beast
REBUILD=false
[ -x $DESTDIR/$BEASTEXE ] || REBUILD=true
if $REBUILD ; then

    # clone/update and build rapicorn
    R=https://github.com/tim-janik/rapicorn.git
    R=../rapicorn/.git/
    git_clone $R rapicorn c013464a64a606fe2165f15396fb96f342d27eb1 # 17.0.0~rc1
    build_checked rapicorn ./autogen.sh --prefix="$BEASTDIR"

    # clone/update and build beast
    R=https://github.com/tim-janik/beast.git
    R=`pwd`/.git/
    git_clone $R beast
    build_checked beast ./autogen.sh --with-pkgroot=/opt --prefix=/usr
fi
[ -x $DESTDIR/$BEASTEXE ] || die "failed to build Beast executable: $BEASTEXE"

NAME="beast"
VERSION=$(./tmpdeb/beast/misc/mkbuildid.sh -p)
GITCOMMIT=`git rev-parse --verify HEAD`
ARCH=$(dpkg --print-architecture)
DUSIZE=$(cd $DESTDIR && du -k -s .)
unset D
D="libc6 (>= 2.15)"
D="$D, libstdc++6 (>= 5.2), zlib1g, libpython2.7, python2.7"
D="$D, libasound2, libflac8 (>= 1.2.1), libfluidsynth1 (>= 1.0.6), libmad0"
D="$D, libogg0, libvorbis0a (>= 1.3.2), libvorbisenc2, libvorbisfile3"
D="$D, libglib2.0-0 (>= 2.32.3), libpango-1.0-0 (>= 1.14.0)"
D="$D, libgdk-pixbuf2.0-0, libgtk2.0-0 (>= 2.12.12), libgnomecanvas2-0 (>= 2.4.0)"
# D="$D, librapicorn-17-0 (>= 17.0.0~rc1), python-rapicorn, rapicorn, librapicorn-dev"

# DEBIAN/control
cat >$DEBIAN/control <<-__EOF
	Package: $NAME
	Version: $VERSION
	License: GNU LGPL v2.1+
	Section: sound
	Priority: optional
	Maintainer: Tim Janik <timj@gnu.org>
	Homepage: https://beast.testbit.eu
	Installed-Size: ${DUSIZE%.}
	Architecture: $ARCH
	Depends: $D
	Description: Music Synthesizer and Composer
	 Beast is a free software music composer and modular synthesizer.
	 .
	 It supports a wide range of standards in the field, such as MIDI,
	 FLAC/WAV/AIFF/MP3/OggVorbis audio files and LADSPA modules.
	 It allows for multitrack editing, unlimited undo/redo support,
	 real-time synthesis support, 32bit audio rendering and MMX/SSE
	 utilisation for synthesis plugins.
	 .
	 This package contains the entire distribution.
__EOF

# DEBIAN/conffiles
echo -n >$DEBIAN/conffiles

# changelog.Debian.gz
DEBCHANGELOG=$DEBDOCDIR/changelog.Debian
rm -f $DEBCHANGELOG.gz $DEBCHANGELOG
DCHCREATE="--create --package $NAME"
for msg in \
  "${NAME^} build: https://github.com/tim-janik/beast/" \
  "git commit $GITCOMMIT" \
  "postinst: add setuid bit to the ${NAME^} launcher, so the audio processing can run at nice level -20 with soft realtime scheduling." \
  ; do
  dch -c $DEBCHANGELOG -v "$VERSION" $DCHCREATE "$msg"
  DCHCREATE=
done
dch -c $DEBCHANGELOG -D stable -r ""
gzip --best $DEBCHANGELOG

# provide mandatory Debian package docs via symlink
(cd $DESTDIR && mkdir -p usr/share/doc/ && rm -f usr/share/doc/$NAME && ln -s $BEASTDIR/doc usr/share/doc/$NAME)

# write out header and shell functions used by postinst or postrm
write_pkgscript_header()
{
  cat <<-\__EOF
	#!/bin/bash
	set -e
	set_perms() {
	  USER="$1"; GROUP="$2"; MODE="$3"; FILE="$4"
	  # https://www.debian.org/doc/debian-policy/ch-files.html#s10.9.1
	  if ! dpkg-statoverride --list "$FILE" > /dev/null ; then
	    chown "$USER:$GROUP" "$FILE"
	    chmod "$MODE" "$FILE"
	  fi
	}
	update_usr_share() {
	  [ ! -x /usr/bin/update-desktop-database ] || update-desktop-database -q /usr/share/applications
	  [ ! -x /usr/bin/update-mime-database ] || update-mime-database /usr/share/mime
	}
__EOF
}

# DEBIAN/postinst
write_pkgscript_header >$DEBIAN/postinst
cat <<-\__EOF |
	# https://www.debian.org/doc/debian-policy/ch-maintainerscripts.html#s-mscriptsinstact
	case "$1" in
	  configure)
	    # https://www.debian.org/doc/debian-policy/ch-files.html#s-permissions-owners
	    set_perms root root 4755 @BEASTEXE@	# wrapper which does renice -20
	    # https://www.debian.org/doc/debian-policy/ch-sharedlibs.html#s-ldconfig
	    ldconfig
	    update_usr_share
	    ;;
	  abort-upgrade|abort-remove|abort-deconfigure)
	    ;;
	  *)
	    # unkown action
	    exit 1
	    ;;
	esac
	exit 0
__EOF
  sed "s|@BEASTEXE@|$BEASTEXE|g" >> $DEBIAN/postinst
chmod +x $DEBIAN/postinst

# DEBIAN/postrm
write_pkgscript_header >$DEBIAN/postrm
cat <<-\__EOF |
	ldconfig
	update_usr_share
	exit 0
__EOF
  cat >>$DEBIAN/postrm
chmod +x $DEBIAN/postrm

# https://wiki.debian.org/ReleaseGoals/LAFileRemoval
find $DEBIAN/../ -name '*.la' -delete

# fix package-installs-python-bytecode
find $DEBIAN/../ -name '*.py[co]' -delete

# build binary deb
fakeroot dpkg-deb -b $DESTDIR $DESTDIR/..

# check the package
#lintian -i --no-tag-display-limit $DESTDIR/../$NAME''_$VERSION''_$ARCH.deb

# Move the package
mv $DESTDIR/../$NAME''_$VERSION''_$ARCH.deb .
ls -al $NAME''_$VERSION''_$ARCH.deb
