#!/bin/bash
# Copyright (C) 2005 Tim Janik
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

IMAGEDIR=`echo "$1" | sed 's,/*$,,'`

[ ! -d "$IMAGEDIR" ] && {
  echo "Usage: $0 [imagedir]" >&2
  exit 1
}

THUMB_SIZE=160x160
GEOMETRY=160x160+8+8

VERBOSE=/dev/null

DEST=`basename $IMAGEDIR`.phpini

rm -f $DEST $IMAGEDIR/*.THUMB.jpg 

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
} | tee $DEST 1>$VERBOSE
