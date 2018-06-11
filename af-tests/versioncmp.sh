#!/bin/bash

# CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0

if test $# -ne 4; then
  echo "Usage: versioncmp.sh VERSION1 VERSION2 OUTPUT1 OUTPUT2"
  echo
  echo "  VERSION1 >= VERSION2  -> print OUTPUT1"
  echo "  else                  -> print OUTPUT2"
  exit 1
fi

if test "x$(echo -e "$1\n$2" | sort -V | head -1)" = "x$2"; then
  echo "$3"
else
  echo "$4"
fi
