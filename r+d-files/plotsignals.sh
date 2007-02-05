#!/bin/bash
#
# plotsignals.sh - plots audio signals with gnuplot to allow visual comparisions
#
# Copyright (C) 2007 Stefan Westerfeld, stefan@space.twc.de
# 
# This software is provided "as is"; redistribution and modification
# is permitted, provided that the following disclaimer is retained.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# In no event shall the authors or contributors be liable for any
# direct, indirect, incidental, special, exemplary, or consequential
# damages (including, but not limited to, procurement of substitute
# goods or services; loss of use, data, or profits; or business
# interruption) however caused and on any theory of liability, whether
# in contract, strict liability, or tort (including negligence or
# otherwise) arising in any way out of the use of this software, even
# if advised of the possibility of such damage.

# defaults
style=''

usage()
{
  echo "plotsignals.sh - plots audio signals with gnuplot to allow visual comparisions"
  echo " "
  echo "plotsignals.sh [options] <list of files>"
  echo " "
  echo "options:"
  echo "-h, --help                show brief help"
  echo "-l, --with-lines          plot signals using lines"
}

# check for options
while test $# -gt 0; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;

    -l|--with-lines)
      style=' with lines'
      shift
      ;;

    *)
      # no more options, the rest are filenames 
      break
      ;;
    esac
done

BSEFEXTRACT=../tools/bsefextract

if test -f $BSEFEXTRACT; then
  if test $# -gt 0; then
    gnuplot <(
      echo -n plot
      comma=''
      for signal in "$@"
      do
	echo -n "$comma" '"< '$BSEFEXTRACT' --cut-zeros --raw-signal ' $signal '"' "$style"
	comma=','
      done
      echo 
      echo 'pause -1'
    )
  else
    usage
    exit 1
  fi
else
  echo "this script needs $BSEFEXTRACT to work"
  exit 2
fi
