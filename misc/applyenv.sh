#!/bin/bash
set -e

# NOTE: avoid using unprefixed environment variables in this script, as they could affect output substitutions

applyenv__SCRIPTNAME___="$(basename "$0")" ; die() { e="$1"; shift; echo "$applyenv__SCRIPTNAME___: $*" >&2; exit "$e"; }

applyenv__input___="$1" ; shift || :

test -r "$applyenv__input___" || die 1 "Usage: applyenv.sh <inputfile.in>"

applyenv__sedprogram___=""
for applyenv__var___ in \
  $(grep -oE '@[0123456789abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ]+@' "$applyenv__input___" | sort | uniq | sed s/@//g)
do
  applyenv__escaped___=$(printf %s "${!applyenv__var___}" | sed 's/[&\\|]/\\&/g') # escape for sed 's' embedding
  applyenv__sedprogram___="$applyenv__sedprogram___; s|@$applyenv__var___@|$applyenv__escaped___|g"
done
set -x
sed "$applyenv__sedprogram___" "$applyenv__input___"
