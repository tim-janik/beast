#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -Eeuo pipefail
set -x

# == Config ==
S=2			# scaling
W=$(( 64 / $S ))	# width
H=$(( 64 / $S ))	# height
SCSSFILES=()

# == Dist basics ==
rm -fr cursors/ cursors-*-g*.tgz
test "${1:-}" != clean || exit
mkdir -p cursors/
pandoc README.md -t markdown -o cursors/README

# == make_cursor ==
make_cursor() # make_cursor foo.svg 16 16
{
  F=$1 && X=$(( $2 / $S )) && Y=$(( $3 / $S ))
  N=${F%.*}	# bare name
  P=cursors/$N.png
  O=cursors/$N.scss
  # Include SVG in output
  cp $F cursors/
  # Render SVG to PNG, render .hotspot.png with hotspot, generate SCSS
  inkscape -z -w $W -h $H -e $P $F
  # Render .hotspot.png with red hotspot
  # http://www.imagemagick.org/Usage/compose/ http://www.imagemagick.org/Usage/draw/
  convert -size $W'x'$H xc:none -draw "fill red point $X,$Y" $O.tmpdot.png
  convert $P $O.tmpdot.png -compose Atop -composite ${P%.png}.hotspot.png
  rm -f $O.tmpdot.png
  # Generate SCSS
  DATA=$(base64 -w0 < $P)
  ( echo "@mixin cursor_$N {"
    echo "  cursor: url(\"data:image/png;base64,$DATA\") $X $Y, auto;"
    echo "}" ) > $O
  SCSSFILES+=($O)
}

# == Cursor List ==
make_cursor	pen.svg		 6 57
make_cursor	eraser.svg	24 57

# == cursors.scss ==
( for P in "${SCSSFILES[@]}"; do
    BASENAME=${P##*/}
    echo "@import './$BASENAME';"
  done ) > cursors/cursors.scss
