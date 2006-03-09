#!/bin/bash
# Copyright (C) 2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.

IMAGEDIR=`echo "$1" | sed 's,/*$,,'`

[ ! -d "$IMAGEDIR" ] && {
  echo "Usage: $0 [imagedir]" >&2
  exit 1
}

THUMB_SIZE=160x160
GEOMETRY=160x160+8+8

# DEST=`basename $IMAGEDIR`.phpini

rm -f $IMAGEDIR/*.THUMB.jpg # $DEST

NTH=1
{
  for i in `find $IMAGEDIR/ -type f | sed 's,^\./\+,,' | sort` ; do
	if `file $i | grep -Fq image` ; then
		echo "[$NTH]"
		identify -format 'width = %w\nheight = %h' "$i"
		echo "fname=$i"
		stat -c 'bytes = %s' "$i"
		THUMB="`echo $i | sed 's/\(\.[^.]\+\)$/.THUMB.jpg/'`"
		echo "tname=$THUMB"
		[ "$THUMB" = "$i" ] && { echo "invalid thumb name: $THUMB" >&2; exit 1; }
		rm -f "$THUMB"
		montage -background \#f6f6f6 -geometry $GEOMETRY -gravity north -resize $THUMB_SIZE "$i" "$THUMB"
		identify -format 'twidth = %w\ntheight = %h' "$THUMB"
		BLURB="`echo $i | sed 's/\(\.[^.]\+\)$/.DSC/'`"
		[ "$BLURB" = "$i" ] && { echo "invalid blurb name: $BLURB" >&2; exit 1; }
		[ -r "$BLURB" ] && {
		  echo -n 'blurb="'
		  sed -e 's/"/\&quot;/g' -e 's/</&lt;/g' < "$BLURB" | tr -t '\n\r' ' '
		  echo '"'
		}
		NTH=$[$NTH + 1]
	fi
  done
}
