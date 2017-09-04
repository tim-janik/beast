#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -e

# Usage: mkbuildid.sh [-p] [ccfile] [hhfile]
# Display BuildID and update buildid() source file

SCRIPTNAME="$(basename "$0")" ; die() { e="$1"; shift; echo "$SCRIPTNAME: $*" >&2; exit "$e"; }
SCRIPTDIR="$(dirname "$(readlink -f "$0")")"

# Extract version, see: https://www.debian.org/doc/debian-policy/ch-controlfields.html#s-f-Version
ACINIT_VERSION="$(sed -nr "/^AC_INIT\b/{ s/^[^,]*,[^0-9]*([A-Za-z0-9.:~+-]*).*/\1/; p; }" $SCRIPTDIR/../configure.ac)"
test -n "$ACINIT_VERSION" || die 7 "failed to detect AC_INIT in $SCRIPTDIR/../configure.ac"

test " $1" = " -p" && { PRINT=true; shift; } || PRINT=false
BUILDID_CC="$1"
BUILDID_HH="$2"
DOTGIT=`git rev-parse --git-dir 2>/dev/null` || true

gen_buildid() {
  # provide $ACINIT_VERSION for plain releases, otherwise
  # provide $ACINIT_VERSION + monotonically increasing suffix + commitid
  DESC=
  COMMITID="${1-HEAD}"
  test -e "$DOTGIT" &&							# Try 'git describe', shallow repos will yield ''
      DESC=$(git describe --match '[0-9]*.*[0-9]' --abbrev=5 $COMMITID 2>/dev/null)
  test -e "$DOTGIT" -a -n "$DESC" ||					# Check for git and git's version info
      { printf %s "${ACINIT_VERSION-0.0.0}+tarball" ; return ; }	# otherwise treat as tarball
  [[ "$DESC" =~ ([0-9]+[.][_#%+0-9.a-z-]*)-([0-9]+-g[a-f0-9]+)$ ]] ||	# Split version from git postfix
      { echo "$ACINIT_VERSION" ; return ; }				# failed to split, we're on a release tag
  # HEAD has commits on top of last release tag, transform e.g. 1.2.3-rc4-7-gabc into increasing suffix
  GPOSTFIX="${BASH_REMATCH[2]}"	# e.g. "7-gabc"
  GPOSTFIX="+${GPOSTFIX//-/.}"	# 7-gabc -> +7.gabc
  printf %s "$ACINIT_VERSION$GPOSTFIX"
}

BUILDID=$(gen_buildid HEAD)

BUILDID_INCLUDE=
test -z "$BUILDID_HH" || BUILDID_INCLUDE="#include \"$BUILDID_HH\""

BUILDID_CODE=$(cat <<__EOF
$BUILDID_INCLUDE
namespace Internal {
  const char* buildid() { return "$BUILDID"; }
}
__EOF
)

if $PRINT ; then
  echo "$BUILDID"
fi

test -z "$BUILDID_CC" || {
  if test ! -e "$BUILDID_CC" || test "$(cat "$BUILDID_CC")" != "$BUILDID_CODE" ; then
    echo "  UPDATE   $BUILDID_CC" >&2
    echo "$BUILDID_CODE" > "$BUILDID_CC"
  fi
}
