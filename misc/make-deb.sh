#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -ex

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
  REPO=$1 ; CONFIGURE="$2"
  ( cd ./tmpdeb/$REPO
    $CONFIGURE --prefix=$PREFIX
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

# TODO: move everything into /opt/beast-<version>/...
# TODO: add /usr/bin/beast -> ../../opt/beast-<version>/bin/beast
# TODO: add /usr/share/doc/beast -> ../../opt/beast-<version>/doc
# TODO: add /usr/share/man/man1/beast.1 -> ../../opt/beast-<version>/doc/beast.1
# TODO: add /usr/share/applications/beast.desktop /usr/share/mime/packages/beast.xml
# TODO: add /usr/share/icons/hicolor/48x48/apps/beast.png ./usr/share/icons/hicolor/scalable/apps/beast.svg
# TODO: add /usr/share/icons/hicolor/scalable/mimetypes/application-bse.svg

# build in ./tmpdeb/
PREFIX=/opt
DESTDIR=`pwd`/tmpdeb/destdir
DESTPREFIX=$DESTDIR$PREFIX
DEBIAN=$DESTDIR/DEBIAN
rm -rf $DEBIAN
mkdir -p $DEBIAN

export PATH="$DESTPREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$DESTPREFIX/lib"
export PKG_CONFIG_RAPICORN_PREFIX=$DESTPREFIX
export PKG_CONFIG_RAPICORN_16_PREFIX=$DESTPREFIX
export PKG_CONFIG_BSE_PREFIX=$DESTPREFIX
export PKG_CONFIG_PATH=$DESTPREFIX/lib/pkgconfig
export AIDACC_DESTDIR=$DESTDIR

echo "CC=$CC CFLAGS=$CFLAGS"
echo "CXX=$CXX CXXFLAGS=$CXXFLAGS"
echo "LDFLAGS=$LDFLAGS DISPLAY=$DISPLAY"
echo "PREFIX=$PREFIX"
echo "DESTDIR=$DESTDIR"
echo "DESTPREFIX=$DESTPREFIX"
echo "PATH=$PATH"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib"
echo "PKG_CONFIG_RAPICORN_PREFIX=$PKG_CONFIG_RAPICORN_PREFIX"
echo "PKG_CONFIG_RAPICORN_16_PREFIX=$PKG_CONFIG_RAPICORN_16_PREFIX"
echo "PKG_CONFIG_BSE_PREFIX=$PKG_CONFIG_BSE_PREFIX"
echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH"
echo "AIDACC_DESTDIR=$AIDACC_DESTDIR"
echo "DEBIAN=$DEBIAN"

REBUILD=false
[ -x $DESTDIR/opt/bin/beast ] || REBUILD=true
if $REBUILD ; then

    # clone/update and build rapicorn
    R=https://github.com/tim-janik/rapicorn.git
    R=../rapicorn/.git/
    git_clone $R rapicorn e7411b30e0b73b421a02c0804f2808cb43db6632
    build_checked rapicorn ./autogen.sh

    # clone/update and build beast
    R=https://github.com/tim-janik/beast.git
    R=`pwd`/.git/
    git_clone $R beast
    build_checked beast ./autogen.sh
fi

NAME="beast"
VERSION=$(./tmpdeb/beast/misc/mkbuildid.sh -p)
GITCOMMIT=`git rev-parse --verify HEAD`
ARCH=$(dpkg --print-architecture)
DUSIZE=$(cd $DESTDIR && du -k -s .)
unset D
D="libc6 (>= 2.15)"
D="$D, libstdc++6 (>= 5.2), zlib1g, libpython2.7, python2.7, guile-1.8-libs"
D="$D, libasound2, libflac8 (>= 1.2.1), libfluidsynth1 (>= 1.0.6), libmad0"
D="$D, libogg0, libvorbis0a (>= 1.3.2), libvorbisenc2, libvorbisfile3"
D="$D, libglib2.0-0 (>= 2.32.3), libpango-1.0-0 (>= 1.14.0)"
D="$D, libgdk-pixbuf2.0-0, libgtk2.0-0 (>= 2.12.12), libgnomecanvas2-0 (>= 2.4.0)"
# D="$D, librapicorn-16-0 (>= 16.0.1~git5374), python-rapicorn, rapicorn, librapicorn-dev"

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

# /opt/share/doc/$NAME/
DEBDOCDIR=$DEBIAN/../opt/share/doc/$NAME/
mkdir -p $DEBDOCDIR

# changelog.Debian.gz
DEBCHANGELOG=$DEBDOCDIR/changelog.Debian
rm -f $DEBCHANGELOG.gz $DEBCHANGELOG
DCHCREATE="--create --package $NAME"
for msg in \
  "${NAME^} build, git commit $GITCOMMIT" \
  "postinst: add setuid bit to the ${NAME^} launcher, so the audio processing can run at nice level -20 with soft realtime scheduling." \
  "Debian package build setup for ${NAME^}, hosted at: https://github.com/tim-janik/beast/" \
  ; do
  dch -c $DEBCHANGELOG -v "$VERSION" $DCHCREATE "$msg"
  DCHCREATE=
done
dch -c $DEBCHANGELOG -D stable -r ""
gzip -9 $DEBCHANGELOG

# copyright
cp debian/copyright $DEBDOCDIR

# DEBIAN/postinst
cp misc/postinst-deb.sh $DEBIAN/postinst

# https://wiki.debian.org/ReleaseGoals/LAFileRemoval
find $DEBIAN/../ -name '*.la' -delete

# fix package-installs-python-bytecode
find $DEBIAN/../ -name '*.py[co]' -delete

# build binary deb
FASTZIP="-Zgzip -z3"
fakeroot dpkg-deb $FASTZIP -b $DESTDIR $DESTDIR/..

# check the package
#lintian -i --no-tag-display-limit $DESTDIR/../$NAME''_$VERSION''_$ARCH.deb

# Move the package
mv $DESTDIR/../$NAME''_$VERSION''_$ARCH.deb .
ls -al $NAME''_$VERSION''_$ARCH.deb
