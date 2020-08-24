#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -Eeuo pipefail
# set -x

# == Config ==
S=1	# scaling
W=32	# width
H=32	# height
ORIGIN=$(pwd)

# == args ==
SCRIPTNAME=${0##*/} ; die() { [ -z "$*" ] || echo "$SCRIPTNAME: $*" >&2; exit 9 ; }
HOTSPOT=false
while test $# -ne 0 ; do
  case "$1" in
    --hotspot)          HOTSPOT=true ;;
    *)			die "unknown argument: '$1'" ;;
  esac
  shift
done

# == Create TMP ==
TMP=$(pwd) && TMP="$TMP/build.$UID"
test -e $TMP/node_modules/.bin/svgtofont || (
  rm -rf $TMP/
  mkdir -p $TMP/
  cd $TMP/
  npm i svgtofont svgo )
rm -rf $TMP/svgs/ $TMP/font/
mkdir -p $TMP/svgs/

# == Dist basics ==
rm -fr beastycons/
test "${1:-}" != clean || exit
mkdir -p beastycons/
pandoc README.md -t markdown -o beastycons/README

# == Optimize Glyph ==
optimize_glyph()
{
  F="$1"
  G="$TMP/svgs/${F##*/}"
  test "$F" = "$G" || cp "$F" "$G"
  $TMP/node_modules/.bin/svgo -i $G --multipass -o $G
}

# == make_cursor ==
make_cursor() # make_cursor foo.svg 16 16 fallback
{
  N=${1%.*}	# bare name
  X=$(( $2 / $S )) && Y=$(( $3 / $S )) && FALLBACK="$4"
  F=cursors/$1
  G=$TMP/svgs/$1
  if $HOTSPOT ; then
    P=beastycons/$N.png
    O=beastycons/$N.tmpdot
    convert -background none $F $P					# Render SVG to PNG
    convert -size $W'x'$H xc:none -draw "fill red point $X,$Y" $O.png	# Render .hotspot.png with red hotspot
    convert $P $O.png -compose Atop -composite ${P%.png}.hotspot.png	# Compose hotspot
    rm -f $O.png $P
  fi
  # Generate SCSS
  URI=$( $TMP/node_modules/.bin/svgo -i $F --multipass --datauri=base64 -o - )
  echo "\$bc-cursor-$N: url($URI) $X $Y, $FALLBACK;" >> beastycons/cursors.tmp
  # Add to font files
  cp $F $G
  inkscape -f $G --verb=EditSelectAll --verb=ObjectToPath --verb=StrokeToPath --verb=FileSave --verb=FileQuit
  inkscape -f $G --vacuum-defs --export-plain-svg $G
  optimize_glyph $G
}
echo > beastycons/cursors.tmp

# == Cursor List ==
make_cursor	pen.svg		 3 28 default
make_cursor	eraser.svg	12 28 not-allowed

# == Populate Glyphs ==
for SVG in icons/*.svg ; do
  optimize_glyph "$SVG"
done

# == Create Font ==
( cd $TMP/
  node_modules/.bin/svgtofont --sources svgs/ --output ./font --fontName Beastycons
  # Keep just woff2 reference in .css
  sed -r '/\burl/s/(Beastycons\.woff2)\?t=[0-9]+/\1/; s/[^ ]*url(.*woff2.*)[,;]/src:URL\1;/; /url/d; s/src:URL/src: url/' -i font/Beastycons.css
)
cp $TMP/font/Beastycons.css $TMP/font/Beastycons.woff2 beastycons/

# == bc-cursors.scss ==
mv beastycons/cursors.tmp beastycons/bc-cursors.scss

# == dist ==
DATE=$(date +%y%m%d)
TARBALL=beastycons-$DATE.1.tgz
tar --mtime="$DATE" -cf $TARBALL --exclude '*.hotspot.png' beastycons/
$HOTSPOT || rm -r beastycons/
tar tvf $TARBALL
ls -l $TARBALL
sha256sum $TARBALL
